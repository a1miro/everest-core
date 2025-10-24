#include "WhiteBeetEthernet.hpp"
#include <everest/logging.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

namespace whitebeet_comms {

WhiteBeetEthernet::WhiteBeetEthernet() 
    : socket_fd(-1), connected(false), running(false) {
}

WhiteBeetEthernet::~WhiteBeetEthernet() {
    disconnect();
}

bool WhiteBeetEthernet::connect(const std::string& ip_address, uint16_t port, uint32_t timeout_ms) {
    if (connected) {
        EVLOG_warning << "WhiteBeet already connected";
        return true;
    }
    
    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        EVLOG_error << "Failed to create socket: " << strerror(errno);
        return false;
    }
    
    // Set socket to non-blocking for timeout control
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        EVLOG_error << "Failed to set socket non-blocking: " << strerror(errno);
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr) <= 0) {
        EVLOG_error << "Invalid IP address: " << ip_address;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // Attempt connection
    int result = ::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (result < 0 && errno != EINPROGRESS) {
        EVLOG_error << "Failed to connect to WhiteBeet at " << ip_address << ":" << port 
                   << " - " << strerror(errno);
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // Wait for connection with timeout
    if (errno == EINPROGRESS) {
        struct pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLOUT;
        
        int poll_result = poll(&pfd, 1, timeout_ms);
        
        if (poll_result <= 0) {
            EVLOG_error << "Connection timeout or error";
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        // Check if connection was successful
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            EVLOG_error << "Connection failed: " << strerror(error);
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
    }
    
    // Set socket back to blocking mode
    flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        EVLOG_warning << "Failed to set socket blocking: " << strerror(errno);
    }
    
    connected = true;
    remote_ip = ip_address;
    remote_port = port;
    
    EVLOG_info << "Successfully connected to WhiteBeet at " << ip_address << ":" << port;
    return true;
}

void WhiteBeetEthernet::disconnect() {
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    connected = false;
    
    EVLOG_info << "Disconnected from WhiteBeet";
}

bool WhiteBeetEthernet::send_message(const std::vector<uint8_t>& message) {
    if (!connected || socket_fd < 0) {
        EVLOG_error << "Not connected to WhiteBeet";
        return false;
    }
    
    size_t total_sent = 0;
    size_t message_size = message.size();
    
    while (total_sent < message_size) {
        ssize_t sent = send(socket_fd, message.data() + total_sent, 
                           message_size - total_sent, MSG_NOSIGNAL);
        
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer full, wait and retry
                usleep(1000); // 1ms delay
                continue;
            } else {
                EVLOG_error << "Failed to send message: " << strerror(errno);
                disconnect();
                return false;
            }
        } else if (sent == 0) {
            EVLOG_error << "Connection closed by peer";
            disconnect();
            return false;
        }
        
        total_sent += sent;
    }
    
    EVLOG_debug << "Sent " << total_sent << " bytes to WhiteBeet";
    return true;
}

bool WhiteBeetEthernet::receive_message(std::vector<uint8_t>& message, uint32_t timeout_ms) {
    if (!connected || socket_fd < 0) {
        EVLOG_error << "Not connected to WhiteBeet";
        return false;
    }
    
    // First, receive the message header to get the length
    std::vector<uint8_t> header(8); // WhiteBeet message header is 8 bytes
    
    if (!receive_exact(header.data(), 8, timeout_ms)) {
        return false;
    }
    
    // Parse message length from header (little endian)
    uint16_t message_length = header[0] | (static_cast<uint16_t>(header[1]) << 8);
    
    if (message_length < 8 || message_length > 65535) {
        EVLOG_error << "Invalid message length: " << message_length;
        return false;
    }
    
    // Prepare message buffer
    message.resize(message_length);
    std::copy(header.begin(), header.end(), message.begin());
    
    // Receive remaining payload if any
    if (message_length > 8) {
        if (!receive_exact(message.data() + 8, message_length - 8, timeout_ms)) {
            return false;
        }
    }
    
    EVLOG_debug << "Received " << message_length << " bytes from WhiteBeet";
    return true;
}

void WhiteBeetEthernet::start_receive_thread(std::function<void(const std::vector<uint8_t>&)> callback) {
    if (running) {
        EVLOG_warning << "Receive thread already running";
        return;
    }
    
    message_callback = callback;
    running = true;
    
    receive_thread = std::thread([this]() {
        EVLOG_info << "WhiteBeet receive thread started";
        
        while (running && connected) {
            std::vector<uint8_t> message;
            
            if (receive_message(message, 1000)) { // 1 second timeout
                if (message_callback) {
                    try {
                        message_callback(message);
                    } catch (const std::exception& e) {
                        EVLOG_error << "Exception in message callback: " << e.what();
                    }
                }
            } else {
                // Check if we're still supposed to be running
                if (running && connected) {
                    EVLOG_debug << "Receive timeout, continuing...";
                }
            }
        }
        
        EVLOG_info << "WhiteBeet receive thread stopped";
    });
}

void WhiteBeetEthernet::stop_receive_thread() {
    if (running) {
        running = false;
        
        if (receive_thread.joinable()) {
            receive_thread.join();
        }
        
        message_callback = nullptr;
        EVLOG_info << "WhiteBeet receive thread stopped";
    }
}

bool WhiteBeetEthernet::is_connected() const {
    return connected;
}

std::string WhiteBeetEthernet::get_remote_address() const {
    if (connected) {
        return remote_ip + ":" + std::to_string(remote_port);
    }
    return "";
}

bool WhiteBeetEthernet::receive_exact(uint8_t* buffer, size_t length, uint32_t timeout_ms) {
    size_t total_received = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (total_received < length) {
        // Check timeout
        if (timeout_ms > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            
            if (elapsed >= timeout_ms) {
                EVLOG_debug << "Receive timeout after " << elapsed << "ms";
                return false;
            }
        }
        
        // Use poll to check if data is available
        struct pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLIN;
        
        int remaining_timeout = timeout_ms > 0 ? 
            (timeout_ms - std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count()) : 1000;
        
        if (remaining_timeout <= 0) remaining_timeout = 1;
        
        int poll_result = poll(&pfd, 1, remaining_timeout);
        
        if (poll_result < 0) {
            EVLOG_error << "Poll error: " << strerror(errno);
            return false;
        } else if (poll_result == 0) {
            // Timeout
            continue;
        }
        
        // Receive data
        ssize_t received = recv(socket_fd, buffer + total_received, 
                               length - total_received, MSG_DONTWAIT);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                EVLOG_error << "Failed to receive data: " << strerror(errno);
                disconnect();
                return false;
            }
        } else if (received == 0) {
            EVLOG_error << "Connection closed by peer";
            disconnect();
            return false;
        }
        
        total_received += received;
    }
    
    return true;
}

} // namespace whitebeet_comms
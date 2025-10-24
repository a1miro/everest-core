// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef ETHERNET_FRAMING_HPP
#define ETHERNET_FRAMING_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

// Network includes
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

namespace whitebeet_comms {

/**
 * @brief Ethernet framing layer for WhiteBeet communication
 * 
 * This class handles low-level Ethernet framing based on the FreeV2G
 * implementation. It can work with both raw Ethernet frames and TCP/IP.
 */
class EthernetFraming {
public:
    using FrameReceivedCallback = std::function<void(const std::vector<uint8_t>& frame)>;
    
    /**
     * @brief Constructor for TCP/IP mode
     * @param interface_name Network interface name
     * @param target_ip Target IP address
     * @param target_port Target TCP port
     */
    EthernetFraming(const std::string& interface_name, 
                   const std::string& target_ip, 
                   int target_port);
    
    /**
     * @brief Constructor for raw Ethernet mode
     * @param interface_name Network interface name
     * @param target_mac Target MAC address
     * @param ethertype Ethernet type field
     */
    EthernetFraming(const std::string& interface_name,
                   const std::string& target_mac,
                   uint16_t ethertype = 0x88B8); // Custom ethertype for WhiteBeet
    
    /**
     * @brief Destructor
     */
    ~EthernetFraming();

    // Connection management
    bool open();
    void close();
    bool is_open() const;

    // Frame transmission
    bool send_frame(const std::vector<uint8_t>& payload);
    bool send_tcp_data(const std::vector<uint8_t>& data);

    // Frame reception
    void set_frame_received_callback(FrameReceivedCallback callback);
    void start_receiving();
    void stop_receiving();

    // Configuration
    bool set_promiscuous_mode(bool enable);
    std::string get_interface_mac() const;
    std::string get_interface_ip() const;

private:
    // Configuration
    std::string interface_name_;
    std::string target_ip_;
    int target_port_;
    std::string target_mac_;
    uint16_t ethertype_;
    bool use_tcp_mode_;
    
    // Network state
    int socket_fd_;
    int interface_index_;
    uint8_t interface_mac_[6];
    uint8_t target_mac_bytes_[6];
    struct sockaddr_in tcp_addr_;
    struct sockaddr_ll eth_addr_;
    
    // Reception
    std::atomic<bool> receiving_;
    std::thread receive_thread_;
    FrameReceivedCallback frame_callback_;
    mutable std::mutex callback_mutex_;
    
    // Private methods
    bool setup_tcp_socket();
    bool setup_raw_socket();
    bool get_interface_info();
    bool parse_mac_address(const std::string& mac_str, uint8_t* mac_bytes);
    std::string format_mac_address(const uint8_t* mac_bytes);
    
    // Frame handling
    void receive_worker();
    bool receive_tcp_frame();
    bool receive_raw_frame();
    void process_received_frame(const std::vector<uint8_t>& frame);
    
    // Ethernet frame construction
    std::vector<uint8_t> build_ethernet_frame(const std::vector<uint8_t>& payload);
    bool extract_payload_from_frame(const std::vector<uint8_t>& frame, std::vector<uint8_t>& payload);
    
    // Utility methods
    void log_debug(const std::string& message);
    void log_error(const std::string& message);
};

/**
 * @brief Ethernet frame structure
 */
struct EthernetFrame {
    uint8_t destination[6];     // Destination MAC
    uint8_t source[6];          // Source MAC
    uint16_t ethertype;         // EtherType field
    std::vector<uint8_t> payload; // Frame payload
    
    EthernetFrame() : ethertype(0) {
        memset(destination, 0, 6);
        memset(source, 0, 6);
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    bool is_valid() const;
    
    // Utility methods
    std::string get_destination_mac_string() const;
    std::string get_source_mac_string() const;
    void set_destination_mac(const std::string& mac);
    void set_source_mac(const std::string& mac);
};

/**
 * @brief TCP frame wrapper for compatibility
 */
struct TcpFrame {
    uint32_t length;            // Payload length
    std::vector<uint8_t> payload; // TCP payload
    
    TcpFrame() : length(0) {}
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    bool is_valid() const;
};

/**
 * @brief Network interface utilities
 */
namespace network_utils {
    std::vector<std::string> get_available_interfaces();
    bool is_interface_up(const std::string& interface_name);
    std::string get_interface_ip(const std::string& interface_name);
    std::string get_interface_mac(const std::string& interface_name);
    bool set_interface_promiscuous(const std::string& interface_name, bool enable);
    int get_interface_index(const std::string& interface_name);
    bool ping_host(const std::string& ip_address, int timeout_ms = 1000);
}

} // namespace whitebeet_comms

#endif // ETHERNET_FRAMING_HPP
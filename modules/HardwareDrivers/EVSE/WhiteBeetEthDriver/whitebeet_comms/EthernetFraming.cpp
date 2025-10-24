#include "EthernetFraming.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <everest/logging.hpp>

namespace whitebeet_comms {

EthernetFraming::EthernetFraming() {
    // Initialize with default frame parameters
}

EthernetFraming::~EthernetFraming() {
}

std::vector<uint8_t> EthernetFraming::create_frame(const std::vector<uint8_t>& payload, 
                                                   const std::array<uint8_t, 6>& dest_mac,
                                                   const std::array<uint8_t, 6>& src_mac,
                                                   uint16_t ethertype) {
    std::vector<uint8_t> frame;
    frame.reserve(14 + payload.size()); // Ethernet header + payload
    
    // Destination MAC (6 bytes)
    frame.insert(frame.end(), dest_mac.begin(), dest_mac.end());
    
    // Source MAC (6 bytes)
    frame.insert(frame.end(), src_mac.begin(), src_mac.end());
    
    // EtherType (2 bytes, big endian)
    uint16_t ethertype_be = htons(ethertype);
    frame.push_back((ethertype_be >> 8) & 0xFF);
    frame.push_back(ethertype_be & 0xFF);
    
    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    
    return frame;
}

bool EthernetFraming::parse_frame(const std::vector<uint8_t>& frame,
                                  std::vector<uint8_t>& payload,
                                  std::array<uint8_t, 6>& dest_mac,
                                  std::array<uint8_t, 6>& src_mac,
                                  uint16_t& ethertype) {
    if (frame.size() < 14) {
        EVLOG_error << "Ethernet frame too short: " << frame.size() << " bytes";
        return false;
    }
    
    // Extract destination MAC
    std::copy(frame.begin(), frame.begin() + 6, dest_mac.begin());
    
    // Extract source MAC
    std::copy(frame.begin() + 6, frame.begin() + 12, src_mac.begin());
    
    // Extract EtherType
    ethertype = (static_cast<uint16_t>(frame[12]) << 8) | frame[13];
    
    // Extract payload
    if (frame.size() > 14) {
        payload.assign(frame.begin() + 14, frame.end());
    } else {
        payload.clear();
    }
    
    return true;
}

std::vector<uint8_t> EthernetFraming::create_tcp_frame(const std::vector<uint8_t>& data,
                                                       uint32_t src_ip, uint16_t src_port,
                                                       uint32_t dest_ip, uint16_t dest_port,
                                                       uint32_t seq_num, uint32_t ack_num,
                                                       uint16_t flags) {
    std::vector<uint8_t> packet;
    
    // IP Header (20 bytes)
    packet.resize(20);
    packet[0] = 0x45;  // Version (4) + IHL (5)
    packet[1] = 0x00;  // DSCP + ECN
    uint16_t total_length = 20 + 20 + data.size();  // IP + TCP + Data
    packet[2] = (total_length >> 8) & 0xFF;
    packet[3] = total_length & 0xFF;
    packet[4] = 0x00; packet[5] = 0x00;  // Identification
    packet[6] = 0x40; packet[7] = 0x00;  // Flags + Fragment offset (Don't fragment)
    packet[8] = 0x40;  // TTL
    packet[9] = 0x06;  // Protocol (TCP)
    packet[10] = 0x00; packet[11] = 0x00;  // Header checksum (calculated later)
    
    // Source IP
    packet[12] = (src_ip >> 24) & 0xFF;
    packet[13] = (src_ip >> 16) & 0xFF;
    packet[14] = (src_ip >> 8) & 0xFF;
    packet[15] = src_ip & 0xFF;
    
    // Destination IP
    packet[16] = (dest_ip >> 24) & 0xFF;
    packet[17] = (dest_ip >> 16) & 0xFF;
    packet[18] = (dest_ip >> 8) & 0xFF;
    packet[19] = dest_ip & 0xFF;
    
    // TCP Header (20 bytes)
    size_t tcp_start = packet.size();
    packet.resize(tcp_start + 20);
    
    // Source port
    packet[tcp_start + 0] = (src_port >> 8) & 0xFF;
    packet[tcp_start + 1] = src_port & 0xFF;
    
    // Destination port
    packet[tcp_start + 2] = (dest_port >> 8) & 0xFF;
    packet[tcp_start + 3] = dest_port & 0xFF;
    
    // Sequence number
    packet[tcp_start + 4] = (seq_num >> 24) & 0xFF;
    packet[tcp_start + 5] = (seq_num >> 16) & 0xFF;
    packet[tcp_start + 6] = (seq_num >> 8) & 0xFF;
    packet[tcp_start + 7] = seq_num & 0xFF;
    
    // Acknowledgment number
    packet[tcp_start + 8] = (ack_num >> 24) & 0xFF;
    packet[tcp_start + 9] = (ack_num >> 16) & 0xFF;
    packet[tcp_start + 10] = (ack_num >> 8) & 0xFF;
    packet[tcp_start + 11] = ack_num & 0xFF;
    
    // Data offset (5 words = 20 bytes) + Reserved + Flags
    packet[tcp_start + 12] = 0x50;  // Data offset: 5 words
    packet[tcp_start + 13] = flags & 0xFF;
    
    // Window size
    packet[tcp_start + 14] = 0xFF;
    packet[tcp_start + 15] = 0xFF;
    
    // Checksum (calculated later)
    packet[tcp_start + 16] = 0x00;
    packet[tcp_start + 17] = 0x00;
    
    // Urgent pointer
    packet[tcp_start + 18] = 0x00;
    packet[tcp_start + 19] = 0x00;
    
    // Add data
    packet.insert(packet.end(), data.begin(), data.end());
    
    return packet;
}

uint16_t EthernetFraming::calculate_checksum(const std::vector<uint8_t>& data, size_t start, size_t length) {
    uint32_t sum = 0;
    
    for (size_t i = start; i < start + length; i += 2) {
        if (i + 1 < data.size()) {
            sum += (static_cast<uint16_t>(data[i]) << 8) + data[i + 1];
        } else {
            sum += static_cast<uint16_t>(data[i]) << 8;
        }
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return static_cast<uint16_t>(~sum);
}

std::string EthernetFraming::mac_to_string(const std::array<uint8_t, 6>& mac) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buffer);
}

std::array<uint8_t, 6> EthernetFraming::string_to_mac(const std::string& mac_str) {
    std::array<uint8_t, 6> mac = {};
    
    if (sscanf(mac_str.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        EVLOG_warning << "Failed to parse MAC address: " << mac_str;
    }
    
    return mac;
}

} // namespace whitebeet_comms
// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef WHITEBEET_ETHERNET_HPP
#define WHITEBEET_ETHERNET_HPP

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>

// Network includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "WhiteBeetProtocol.hpp"
#include "EthernetFraming.hpp"

namespace whitebeet_comms {

// Forward declarations
class EthernetFraming;
struct WhiteBeetMessage;

/**
 * @brief Main class for WhiteBeet Ethernet communication
 * 
 * This class implements the communication protocol used by the 8devices WHITE-beet-EI
 * ISO15118 EVSE module over Ethernet. It's based on the FreeV2G reference implementation
 * from Sevenstax.
 */
class WhiteBeetEthernet {
public:
    // Callback function types
    using SlacMatchedCallback = std::function<void(const std::string& ev_mac_address)>;
    using SlacUnmatchedCallback = std::function<void()>;
    using V2GSessionStartedCallback = std::function<void(const std::string& session_id, const std::string& protocol)>;
    using V2GSessionStoppedCallback = std::function<void()>;
    using ChargingStartedCallback = std::function<void()>;
    using ChargingStoppedCallback = std::function<void()>;
    using PowerMeasurementCallback = std::function<void(double voltage, double current, double power, double energy)>;
    using RcdFaultCallback = std::function<void(bool fault)>;
    using ConnectorLockCallback = std::function<void(bool locked)>;
    using ControlPilotCallback = std::function<void(const std::string& state)>;
    using ErrorCallback = std::function<void(const std::string& error_msg)>;

    /**
     * @brief Constructor
     * @param interface_name Network interface name (e.g., "eth0")
     * @param mac_address MAC address of the WhiteBeet module
     * @param ip_address IP address of the WhiteBeet module
     * @param port TCP port for communication
     * @param debug_mode Enable debug logging
     */
    WhiteBeetEthernet(const std::string& interface_name,
                      const std::string& mac_address,
                      const std::string& ip_address,
                      int port,
                      bool debug_mode = false);

    /**
     * @brief Destructor
     */
    ~WhiteBeetEthernet();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool ping();

    // Configuration methods
    bool configure_evse(const std::string& evse_id,
                       int max_current_A,
                       int min_current_A,
                       int max_voltage_V,
                       int min_voltage_V,
                       int max_power_W,
                       int phases);

    // Control methods
    bool enable_control_pilot();
    bool disable_control_pilot();
    bool set_control_pilot_duty_cycle(double percentage);
    bool start_slac();
    bool stop_slac();
    bool start_v2g_session();
    bool stop_v2g_session();
    bool enable_charging();
    bool disable_charging();
    bool lock_connector();
    bool unlock_connector();

    // Status query methods
    bool get_slac_status(std::string& state, std::string& ev_mac);
    bool get_v2g_status(std::string& session_id, std::string& protocol, bool& active);
    bool get_power_measurement(double& voltage, double& current, double& power, double& energy);
    bool get_rcd_status(bool& fault);
    bool get_connector_lock_status(bool& locked);
    bool get_control_pilot_status(std::string& state, double& duty_cycle);

    // Callback setters
    void set_slac_matched_callback(SlacMatchedCallback callback);
    void set_slac_unmatched_callback(SlacUnmatchedCallback callback);
    void set_v2g_session_started_callback(V2GSessionStartedCallback callback);
    void set_v2g_session_stopped_callback(V2GSessionStoppedCallback callback);
    void set_charging_started_callback(ChargingStartedCallback callback);
    void set_charging_stopped_callback(ChargingStoppedCallback callback);
    void set_power_measurement_callback(PowerMeasurementCallback callback);
    void set_rcd_fault_callback(RcdFaultCallback callback);
    void set_connector_lock_callback(ConnectorLockCallback callback);
    void set_control_pilot_callback(ControlPilotCallback callback);
    void set_error_callback(ErrorCallback callback);

private:
    // Configuration
    std::string interface_name_;
    std::string mac_address_;
    std::string ip_address_;
    int port_;
    bool debug_mode_;

    // Connection state
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    
    // Network communication
    std::unique_ptr<EthernetFraming> ethernet_framing_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
    
    // Threading
    std::thread receive_thread_;
    std::thread process_thread_;
    mutable std::mutex send_mutex_;
    mutable std::mutex receive_mutex_;
    
    // Message queues
    std::queue<WhiteBeetMessage> outgoing_messages_;
    std::queue<WhiteBeetMessage> incoming_messages_;
    std::mutex outgoing_queue_mutex_;
    std::mutex incoming_queue_mutex_;
    
    // Callbacks
    SlacMatchedCallback slac_matched_callback_;
    SlacUnmatchedCallback slac_unmatched_callback_;
    V2GSessionStartedCallback v2g_session_started_callback_;
    V2GSessionStoppedCallback v2g_session_stopped_callback_;
    ChargingStartedCallback charging_started_callback_;
    ChargingStoppedCallback charging_stopped_callback_;
    PowerMeasurementCallback power_measurement_callback_;
    RcdFaultCallback rcd_fault_callback_;
    ConnectorLockCallback connector_lock_callback_;
    ControlPilotCallback control_pilot_callback_;
    ErrorCallback error_callback_;
    
    // State tracking
    std::string current_slac_state_;
    std::string current_ev_mac_;
    std::string current_session_id_;
    std::string current_protocol_;
    bool v2g_session_active_;
    bool charging_active_;
    bool connector_locked_;
    bool rcd_fault_;
    std::string control_pilot_state_;
    double control_pilot_duty_cycle_;
    
    // Power measurement data
    double voltage_;
    double current_;
    double power_;
    double energy_;
    
    // Private methods
    bool setup_socket();
    void cleanup_socket();
    bool send_raw_data(const std::vector<uint8_t>& data);
    bool send_message(const WhiteBeetMessage& message);
    void receive_worker();
    void process_worker();
    void process_received_message(const WhiteBeetMessage& message);
    void handle_slac_message(const WhiteBeetMessage& message);
    void handle_v2g_message(const WhiteBeetMessage& message);
    void handle_power_message(const WhiteBeetMessage& message);
    void handle_control_message(const WhiteBeetMessage& message);
    void handle_status_message(const WhiteBeetMessage& message);
    void handle_error_message(const WhiteBeetMessage& message);
    
    // Utility methods
    bool parse_mac_address(const std::string& mac_str, uint8_t* mac_bytes);
    std::string format_mac_address(const uint8_t* mac_bytes);
    void log_debug(const std::string& message);
    void log_info(const std::string& message);
    void log_warning(const std::string& message);
    void log_error(const std::string& message);
    
    // Message creation helpers
    WhiteBeetMessage create_ping_message();
    WhiteBeetMessage create_config_message(const std::string& evse_id,
                                         int max_current_A, int min_current_A,
                                         int max_voltage_V, int min_voltage_V,
                                         int max_power_W, int phases);
    WhiteBeetMessage create_control_pilot_message(bool enable, double duty_cycle = 0.0);
    WhiteBeetMessage create_slac_control_message(bool start);
    WhiteBeetMessage create_charging_control_message(bool enable);
    WhiteBeetMessage create_connector_lock_message(bool lock);
    WhiteBeetMessage create_status_request_message(const std::string& component);
    
    // Response handling
    bool wait_for_response(uint16_t message_id, WhiteBeetMessage& response, int timeout_ms = 5000);
    void notify_error(const std::string& error_msg);
};

} // namespace whitebeet_comms

#endif // WHITEBEET_ETHERNET_HPP
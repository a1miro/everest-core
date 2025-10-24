// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef WHITEBEET_ETH_DRIVER_HPP
#define WHITEBEET_ETH_DRIVER_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/ISO15118_charger/Implementation.hpp>
#include <generated/interfaces/ac_rcd/Implementation.hpp>
#include <generated/interfaces/connector_lock/Implementation.hpp>
#include <generated/interfaces/evse_board_support/Implementation.hpp>
#include <generated/interfaces/powermeter/Implementation.hpp>
#include <generated/interfaces/slac/Implementation.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
#include "whitebeet_comms/WhiteBeetEthernet.hpp"
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    std::string ethernet_interface;
    std::string mac_address;
    std::string ip_address;
    int port;
    int connection_timeout_s;
    int retry_interval_s;
    int max_retries;
    int connector_id;
    std::string evse_id;
    int max_current_A;
    int min_current_A;
    int max_voltage_V;
    int min_voltage_V;
    int max_power_W;
    int phases;
    bool has_rcd;
    bool has_connector_lock;
    bool enable_control_pilot;
    bool debug_mode;
};

class WhiteBeetEthDriver : public Everest::ModuleBase {
public:
    WhiteBeetEthDriver() = delete;
    WhiteBeetEthDriver(const ModuleInfo& info, Everest::TelemetryProvider& telemetry,
                       std::unique_ptr<ISO15118_chargerImplBase> p_charger,
                       std::unique_ptr<slacImplBase> p_slac,
                       std::unique_ptr<evse_board_supportImplBase> p_board_support,
                       std::unique_ptr<powermeterImplBase> p_powermeter,
                       std::unique_ptr<ac_rcdImplBase> p_rcd,
                       std::unique_ptr<connector_lockImplBase> p_connector_lock,
                       Conf& config) :
        ModuleBase(info),
        telemetry(telemetry),
        p_charger(std::move(p_charger)),
        p_slac(std::move(p_slac)),
        p_board_support(std::move(p_board_support)),
        p_powermeter(std::move(p_powermeter)),
        p_rcd(std::move(p_rcd)),
        p_connector_lock(std::move(p_connector_lock)),
        config(config) {};

    Everest::TelemetryProvider& telemetry;
    const std::unique_ptr<ISO15118_chargerImplBase> p_charger;
    const std::unique_ptr<slacImplBase> p_slac;
    const std::unique_ptr<evse_board_supportImplBase> p_board_support;
    const std::unique_ptr<powermeterImplBase> p_powermeter;
    const std::unique_ptr<ac_rcdImplBase> p_rcd;
    const std::unique_ptr<connector_lockImplBase> p_connector_lock;
    const Conf& config;

    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1
    // Public methods for inter-module communication
    void publish_telemetry_data(const std::string& topic, const Everest::TelemetryMap& data);
    
    // WhiteBeet communication handler
    std::unique_ptr<whitebeet_comms::WhiteBeetEthernet> whitebeet_handler;
    
    // State management
    std::atomic<bool> slac_matched{false};
    std::atomic<bool> v2g_session_active{false};
    std::atomic<bool> charging_active{false};
    std::atomic<bool> connector_locked{false};
    std::atomic<bool> rcd_fault{false};
    
    // Mutex for thread-safe operations
    mutable std::mutex state_mutex;
    
    // Callback handlers for WhiteBeet events
    void on_slac_matched(const std::string& ev_mac_address);
    void on_slac_unmatched();
    void on_v2g_session_started(const std::string& session_id, const std::string& protocol);
    void on_v2g_session_stopped();
    void on_charging_started();
    void on_charging_stopped();
    void on_power_measurement(double voltage, double current, double power, double energy);
    void on_rcd_fault(bool fault);
    void on_connector_lock_state(bool locked);
    void on_control_pilot_state(const std::string& state);
    
    // Error handling
    void handle_communication_error(const std::string& error_msg);
    void clear_errors();
    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1

protected:
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1
    // insert your protected definitions here
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1

private:
    friend class LdEverest;
    void init();
    void ready();

    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
    // Private members for module functionality
    
    // Telemetry data containers
    Everest::TelemetryMap telemetry_power_data;
    Everest::TelemetryMap telemetry_slac_data;
    Everest::TelemetryMap telemetry_v2g_data;
    Everest::TelemetryMap telemetry_system_data;
    std::mutex telemetry_mutex;
    
    // Background thread for telemetry publishing
    std::thread telemetry_thread;
    std::atomic<bool> telemetry_thread_running{false};
    void telemetry_worker();
    
    // Communication monitoring
    std::thread comm_monitor_thread;
    std::atomic<bool> comm_monitor_running{false};
    void communication_monitor();
    
    // Connection state
    std::atomic<bool> connected_to_whitebeet{false};
    std::atomic<bool> initialization_complete{false};
    
    // Error state tracking
    std::atomic<bool> last_rcd_state{false};
    std::atomic<bool> last_connector_lock_state{false};
    std::string last_cp_state{"A"};
    std::string current_ev_mac_address;
    std::string current_session_id;
    std::string current_protocol;
    
    // Connection management
    bool establish_connection();
    void cleanup_connection();
    bool is_whitebeet_responsive();
    void reset_whitebeet_module();
    
    // Configuration validation
    bool validate_configuration();
    
    // Utility functions
    std::string mac_address_to_string(const uint8_t* mac);
    bool parse_mac_address(const std::string& mac_str, uint8_t* mac_bytes);
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// Utility functions for data conversion and formatting
namespace whitebeet_utils {
    std::string format_evse_id(const std::string& evse_id);
    std::string format_session_id(const std::string& session_id);
    types::powermeter::Powermeter create_powermeter_data(double voltage, double current, double power, double energy);
    types::board_support_common::BspEvent create_bsp_event(const std::string& event_type);
    types::slac::EvseMatchingResults create_slac_results(const std::string& ev_mac);
}
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // WHITEBEET_ETH_DRIVER_HPP
// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include "WhiteBeetEthDriver.hpp"

#include <everest/logging.hpp>
#include <chrono>
#include <thread>

namespace module {

void WhiteBeetEthDriver::init() {
    // Initialize logging for this module
    EVLOG_info << "Initializing WhiteBeetEthDriver module";
    
    // Validate configuration
    if (!validate_configuration()) {
        EVLOG_error << "Configuration validation failed";
        return;
    }
    
    // Initialize WhiteBeet communication handler
    try {
        whitebeet_handler = std::make_unique<whitebeet_comms::WhiteBeetEthernet>(
            config.ethernet_interface,
            config.mac_address,
            config.ip_address,
            config.port,
            config.debug_mode
        );
        
        // Set up callback handlers
        whitebeet_handler->set_slac_matched_callback([this](const std::string& ev_mac) {
            this->on_slac_matched(ev_mac);
        });
        
        whitebeet_handler->set_slac_unmatched_callback([this]() {
            this->on_slac_unmatched();
        });
        
        whitebeet_handler->set_v2g_session_started_callback([this](const std::string& session_id, const std::string& protocol) {
            this->on_v2g_session_started(session_id, protocol);
        });
        
        whitebeet_handler->set_v2g_session_stopped_callback([this]() {
            this->on_v2g_session_stopped();
        });
        
        whitebeet_handler->set_charging_started_callback([this]() {
            this->on_charging_started();
        });
        
        whitebeet_handler->set_charging_stopped_callback([this]() {
            this->on_charging_stopped();
        });
        
        whitebeet_handler->set_power_measurement_callback([this](double v, double i, double p, double e) {
            this->on_power_measurement(v, i, p, e);
        });
        
        whitebeet_handler->set_rcd_fault_callback([this](bool fault) {
            this->on_rcd_fault(fault);
        });
        
        whitebeet_handler->set_connector_lock_callback([this](bool locked) {
            this->on_connector_lock_state(locked);
        });
        
        whitebeet_handler->set_control_pilot_callback([this](const std::string& state) {
            this->on_control_pilot_state(state);
        });
        
        whitebeet_handler->set_error_callback([this](const std::string& error) {
            this->handle_communication_error(error);
        });
        
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to initialize WhiteBeet handler: " << e.what();
        return;
    }
    
    // Initialize interface implementations
    invoke_init(*p_charger);
    invoke_init(*p_slac);
    invoke_init(*p_board_support);
    invoke_init(*p_powermeter);
    invoke_init(*p_rcd);
    invoke_init(*p_connector_lock);
    
    EVLOG_info << "WhiteBeetEthDriver initialization completed";
}

void WhiteBeetEthDriver::ready() {
    EVLOG_info << "WhiteBeetEthDriver module ready";
    
    // Establish connection to WhiteBeet module
    if (!establish_connection()) {
        EVLOG_error << "Failed to establish connection to WhiteBeet module";
        return;
    }
    
    // Start telemetry thread
    telemetry_thread_running = true;
    telemetry_thread = std::thread(&WhiteBeetEthDriver::telemetry_worker, this);
    
    // Start communication monitor thread
    comm_monitor_running = true;
    comm_monitor_thread = std::thread(&WhiteBeetEthDriver::communication_monitor, this);
    
    // Invoke ready on interface implementations
    invoke_ready(*p_charger);
    invoke_ready(*p_slac);
    invoke_ready(*p_board_support);
    invoke_ready(*p_powermeter);
    invoke_ready(*p_rcd);
    invoke_ready(*p_connector_lock);
    
    initialization_complete = true;
    EVLOG_info << "WhiteBeetEthDriver is ready and operational";
}

bool WhiteBeetEthDriver::establish_connection() {
    EVLOG_info << "Establishing connection to WhiteBeet module at " << config.ip_address << ":" << config.port;
    
    int retry_count = 0;
    while (retry_count < config.max_retries) {
        try {
            if (whitebeet_handler->connect()) {
                EVLOG_info << "Successfully connected to WhiteBeet module";
                connected_to_whitebeet = true;
                
                // Configure WhiteBeet module with EVSE parameters
                whitebeet_handler->configure_evse(
                    config.evse_id,
                    config.max_current_A,
                    config.min_current_A,
                    config.max_voltage_V,
                    config.min_voltage_V,
                    config.max_power_W,
                    config.phases
                );
                
                // Enable control pilot if configured
                if (config.enable_control_pilot) {
                    whitebeet_handler->enable_control_pilot();
                }
                
                return true;
            }
        } catch (const std::exception& e) {
            EVLOG_warning << "Connection attempt " << (retry_count + 1) 
                         << " failed: " << e.what();
        }
        
        retry_count++;
        if (retry_count < config.max_retries) {
            EVLOG_info << "Retrying connection in " << config.retry_interval_s << " seconds...";
            std::this_thread::sleep_for(std::chrono::seconds(config.retry_interval_s));
        }
    }
    
    EVLOG_error << "Failed to connect to WhiteBeet module after " << config.max_retries << " attempts";
    return false;
}

void WhiteBeetEthDriver::cleanup_connection() {
    if (whitebeet_handler && connected_to_whitebeet) {
        whitebeet_handler->disconnect();
        connected_to_whitebeet = false;
    }
}

void WhiteBeetEthDriver::telemetry_worker() {
    EVLOG_debug << "Telemetry worker thread started";
    
    while (telemetry_thread_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!connected_to_whitebeet || !initialization_complete) {
            continue;
        }
        
        // Publish system telemetry
        {
            std::lock_guard<std::mutex> lock(telemetry_mutex);
            telemetry_system_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            telemetry_system_data["connected"] = connected_to_whitebeet.load();
            telemetry_system_data["slac_matched"] = slac_matched.load();
            telemetry_system_data["v2g_session_active"] = v2g_session_active.load();
            telemetry_system_data["charging_active"] = charging_active.load();
            telemetry_system_data["connector_locked"] = connector_locked.load();
            telemetry_system_data["rcd_fault"] = rcd_fault.load();
            telemetry_system_data["ev_mac_address"] = current_ev_mac_address;
            telemetry_system_data["session_id"] = current_session_id;
            telemetry_system_data["protocol"] = current_protocol;
            
            publish_telemetry_data("whitebeet_system", telemetry_system_data);
        }
    }
    
    EVLOG_debug << "Telemetry worker thread stopped";
}

void WhiteBeetEthDriver::communication_monitor() {
    EVLOG_debug << "Communication monitor thread started";
    
    while (comm_monitor_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (connected_to_whitebeet && !is_whitebeet_responsive()) {
            EVLOG_warning << "WhiteBeet module is not responsive, attempting reconnection";
            cleanup_connection();
            establish_connection();
        }
    }
    
    EVLOG_debug << "Communication monitor thread stopped";
}

bool WhiteBeetEthDriver::is_whitebeet_responsive() {
    if (!whitebeet_handler) {
        return false;
    }
    
    try {
        return whitebeet_handler->ping();
    } catch (const std::exception& e) {
        EVLOG_warning << "Ping to WhiteBeet failed: " << e.what();
        return false;
    }
}

void WhiteBeetEthDriver::publish_telemetry_data(const std::string& topic, const Everest::TelemetryMap& data) {
    telemetry.publish(topic, data);
}

bool WhiteBeetEthDriver::validate_configuration() {
    // Validate MAC address format
    uint8_t mac_bytes[6];
    if (!parse_mac_address(config.mac_address, mac_bytes)) {
        EVLOG_error << "Invalid MAC address format: " << config.mac_address;
        return false;
    }
    
    // Validate current ranges
    if (config.min_current_A > config.max_current_A) {
        EVLOG_error << "Minimum current (" << config.min_current_A 
                   << "A) is greater than maximum current (" << config.max_current_A << "A)";
        return false;
    }
    
    // Validate voltage ranges
    if (config.min_voltage_V > config.max_voltage_V) {
        EVLOG_error << "Minimum voltage (" << config.min_voltage_V 
                   << "V) is greater than maximum voltage (" << config.max_voltage_V << "V)";
        return false;
    }
    
    // Validate phases
    if (config.phases < 1 || config.phases > 3) {
        EVLOG_error << "Invalid phase count: " << config.phases << " (must be 1-3)";
        return false;
    }
    
    EVLOG_info << "Configuration validation passed";
    return true;
}

bool WhiteBeetEthDriver::parse_mac_address(const std::string& mac_str, uint8_t* mac_bytes) {
    if (mac_str.length() != 17) { // Format: XX:XX:XX:XX:XX:XX
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        std::string byte_str = mac_str.substr(i * 3, 2);
        try {
            mac_bytes[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        } catch (const std::exception&) {
            return false;
        }
        
        if (i < 5 && mac_str[i * 3 + 2] != ':') {
            return false;
        }
    }
    
    return true;
}

std::string WhiteBeetEthDriver::mac_address_to_string(const uint8_t* mac) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(mac_str);
}

// Callback handlers for WhiteBeet events
void WhiteBeetEthDriver::on_slac_matched(const std::string& ev_mac_address) {
    EVLOG_info << "SLAC matched with EV: " << ev_mac_address;
    slac_matched = true;
    current_ev_mac_address = ev_mac_address;
    
    // Publish SLAC state change
    p_slac->publish_state("MATCHED");
    p_slac->publish_dlink_ready(true);
    p_slac->publish_ev_mac_address(ev_mac_address);
    
    // Update telemetry
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    telemetry_slac_data["state"] = "MATCHED";
    telemetry_slac_data["ev_mac"] = ev_mac_address;
    telemetry_slac_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    publish_telemetry_data("whitebeet_slac", telemetry_slac_data);
}

void WhiteBeetEthDriver::on_slac_unmatched() {
    EVLOG_info << "SLAC unmatched";
    slac_matched = false;
    current_ev_mac_address.clear();
    
    // Publish SLAC state change
    p_slac->publish_state("UNMATCHED");
    p_slac->publish_dlink_ready(false);
    
    // Update telemetry
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    telemetry_slac_data["state"] = "UNMATCHED";
    telemetry_slac_data["ev_mac"] = "";
    telemetry_slac_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    publish_telemetry_data("whitebeet_slac", telemetry_slac_data);
}

void WhiteBeetEthDriver::on_v2g_session_started(const std::string& session_id, const std::string& protocol) {
    EVLOG_info << "V2G session started - ID: " << session_id << ", Protocol: " << protocol;
    v2g_session_active = true;
    current_session_id = session_id;
    current_protocol = protocol;
    
    // Publish V2G session event
    types::iso15118_charger::SessionEvent session_event;
    session_event.event = types::iso15118_charger::SessionEventEnum::SessionStarted;
    session_event.session_id = session_id;
    p_charger->publish_V2G_Session_Event(session_event);
    
    // Update telemetry
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    telemetry_v2g_data["session_active"] = true;
    telemetry_v2g_data["session_id"] = session_id;
    telemetry_v2g_data["protocol"] = protocol;
    telemetry_v2g_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    publish_telemetry_data("whitebeet_v2g", telemetry_v2g_data);
}

void WhiteBeetEthDriver::on_v2g_session_stopped() {
    EVLOG_info << "V2G session stopped";
    v2g_session_active = false;
    charging_active = false;
    
    // Publish V2G session event
    types::iso15118_charger::SessionEvent session_event;
    session_event.event = types::iso15118_charger::SessionEventEnum::SessionStopped;
    session_event.session_id = current_session_id;
    p_charger->publish_V2G_Session_Event(session_event);
    
    current_session_id.clear();
    current_protocol.clear();
    
    // Update telemetry
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    telemetry_v2g_data["session_active"] = false;
    telemetry_v2g_data["session_id"] = "";
    telemetry_v2g_data["protocol"] = "";
    telemetry_v2g_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    publish_telemetry_data("whitebeet_v2g", telemetry_v2g_data);
}

void WhiteBeetEthDriver::on_charging_started() {
    EVLOG_info << "Charging started";
    charging_active = true;
    
    // Publish charging event
    p_board_support->publish_event(whitebeet_utils::create_bsp_event("ChargingStarted"));
}

void WhiteBeetEthDriver::on_charging_stopped() {
    EVLOG_info << "Charging stopped";
    charging_active = false;
    
    // Publish charging event
    p_board_support->publish_event(whitebeet_utils::create_bsp_event("ChargingStopped"));
}

void WhiteBeetEthDriver::on_power_measurement(double voltage, double current, double power, double energy) {
    // Publish power measurement data
    types::powermeter::Powermeter power_data = whitebeet_utils::create_powermeter_data(voltage, current, power, energy);
    p_powermeter->publish_powermeter(power_data);
    
    // Update telemetry
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    telemetry_power_data["voltage"] = voltage;
    telemetry_power_data["current"] = current;
    telemetry_power_data["power"] = power;
    telemetry_power_data["energy"] = energy;
    telemetry_power_data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    publish_telemetry_data("whitebeet_power", telemetry_power_data);
}

void WhiteBeetEthDriver::on_rcd_fault(bool fault) {
    if (rcd_fault != fault) {
        EVLOG_info << "RCD fault state changed: " << (fault ? "FAULT" : "OK");
        rcd_fault = fault;
        last_rcd_state = fault;
        
        // Publish RCD event
        types::ac_rcd::RcdState rcd_state;
        rcd_state.rcd_current_mA = fault ? 30.0 : 0.0; // Assuming 30mA fault threshold
        p_rcd->publish_rcd_current_state(rcd_state);
    }
}

void WhiteBeetEthDriver::on_connector_lock_state(bool locked) {
    if (connector_locked != locked) {
        EVLOG_info << "Connector lock state changed: " << (locked ? "LOCKED" : "UNLOCKED");
        connector_locked = locked;
        last_connector_lock_state = locked;
        
        // Publish connector lock event
        types::connector_lock::ConnectorLockState lock_state;
        lock_state.connector_lock = locked ? types::connector_lock::ConnectorLockEnum::Locked 
                                          : types::connector_lock::ConnectorLockEnum::Unlocked;
        p_connector_lock->publish_state(lock_state);
    }
}

void WhiteBeetEthDriver::on_control_pilot_state(const std::string& state) {
    if (last_cp_state != state) {
        EVLOG_info << "Control Pilot state changed: " << state;
        last_cp_state = state;
        
        // Publish CP state change
        p_board_support->publish_event(whitebeet_utils::create_bsp_event("CpStateChanged"));
    }
}

void WhiteBeetEthDriver::handle_communication_error(const std::string& error_msg) {
    EVLOG_error << "WhiteBeet communication error: " << error_msg;
    
    // Publish error event
    p_board_support->publish_event(whitebeet_utils::create_bsp_event("CommunicationError"));
    
    // Attempt reconnection
    connected_to_whitebeet = false;
}

void WhiteBeetEthDriver::clear_errors() {
    rcd_fault = false;
    last_rcd_state = false;
    EVLOG_info << "Errors cleared";
}

// Utility functions
namespace whitebeet_utils {

std::string format_evse_id(const std::string& evse_id) {
    return evse_id;
}

std::string format_session_id(const std::string& session_id) {
    return session_id;
}

types::powermeter::Powermeter create_powermeter_data(double voltage, double current, double power, double energy) {
    types::powermeter::Powermeter pm;
    pm.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    
    // Set voltage
    pm.voltage_V.DC = voltage;
    
    // Set current
    pm.current_A.DC = current;
    
    // Set power
    pm.power_W.total = power;
    
    // Set energy
    pm.energy_Wh_import.total = energy;
    
    return pm;
}

types::board_support_common::BspEvent create_bsp_event(const std::string& event_type) {
    types::board_support_common::BspEvent event;
    event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    
    if (event_type == "ChargingStarted") {
        event.event = types::board_support_common::BspEventEnum::A_F;
    } else if (event_type == "ChargingStopped") {
        event.event = types::board_support_common::BspEventEnum::F_A;
    } else if (event_type == "CpStateChanged") {
        event.event = types::board_support_common::BspEventEnum::A_F;
    } else if (event_type == "CommunicationError") {
        event.event = types::board_support_common::BspEventEnum::Error_DF;
    } else {
        event.event = types::board_support_common::BspEventEnum::A_F;
    }
    
    return event;
}

types::slac::EvseMatchingResults create_slac_results(const std::string& ev_mac) {
    types::slac::EvseMatchingResults results;
    results.ev_mac_address = ev_mac;
    return results;
}

} // namespace whitebeet_utils

} // namespace module
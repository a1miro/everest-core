// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "evse_board_supportImpl.hpp"

#include <everest/logging.hpp>

namespace module {
namespace board_support {

void evse_board_supportImpl::init() {
    // Initialize board support implementation
    EVLOG_debug << "Initializing WhiteBeet EVSE board support";
    
    enabled_ = false;
    power_on_allowed_ = false;
    current_pwm_duty_cycle_ = 0.0;
    three_phase_mode_ = true; // Default to 3-phase
    overcurrent_limit_A_ = 32.0; // Default limit
    
    // Initialize proximity pilot reading
    last_pp_reading_.ampacity = types::board_support_common::Ampacity::A_32;
    last_pp_reading_.proximity_resistor = types::board_support_common::ProximityResistor::unknown;
    
    EVLOG_debug << "WhiteBeet EVSE board support initialized";
}

void evse_board_supportImpl::ready() {
    EVLOG_debug << "WhiteBeet EVSE board support ready";
    
    // Update initial capabilities
    update_capabilities();
    
    // Disable control pilot initially
    set_control_pilot_mode(false);
}

void evse_board_supportImpl::handle_enable(bool& value) {
    EVLOG_debug << "Enable EVSE: " << (value ? "true" : "false");
    
    enabled_ = value;
    
    if (!set_control_pilot_mode(value)) {
        EVLOG_error << "Failed to " << (value ? "enable" : "disable") << " control pilot";
        // Don't throw here, just log the error
        return;
    }
    
    // Publish capabilities when enabled
    if (enabled_) {
        update_capabilities();
    }
}

void evse_board_supportImpl::handle_pwm_on(double& value) {
    EVLOG_debug << "PWM on with duty cycle: " << value << "%";
    
    // Clamp duty cycle to valid range (5% - 97%)
    if (value < 5.0) {
        value = 5.0;
    } else if (value > 97.0) {
        value = 97.0;
    }
    
    current_pwm_duty_cycle_ = value;
    
    if (!set_duty_cycle(value)) {
        EVLOG_error << "Failed to set PWM duty cycle to " << value << "%";
        return;
    }
    
    // Publish BSP event for PWM state change
    types::board_support_common::BspEvent event;
    event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    event.event = types::board_support_common::BspEventEnum::B_C;
    publish_event(event);
}

void evse_board_supportImpl::handle_pwm_off() {
    EVLOG_debug << "PWM off";
    
    current_pwm_duty_cycle_ = 100.0; // 100% = off
    
    if (!set_duty_cycle(100.0)) {
        EVLOG_error << "Failed to turn off PWM";
        return;
    }
    
    // Publish BSP event for PWM state change
    types::board_support_common::BspEvent event;
    event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    event.event = types::board_support_common::BspEventEnum::C_A;
    publish_event(event);
}

void evse_board_supportImpl::handle_pwm_F() {
    EVLOG_debug << "PWM F (Error state)";
    
    current_pwm_duty_cycle_ = 0.0; // 0% = error state
    
    if (!set_duty_cycle(0.0)) {
        EVLOG_error << "Failed to set PWM to error state";
        return;
    }
    
    // Publish BSP event for error state
    types::board_support_common::BspEvent event;
    event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    event.event = types::board_support_common::BspEventEnum::Error_F;
    publish_event(event);
}

void evse_board_supportImpl::handle_allow_power_on(types::evse_board_support::PowerOnOff& value) {
    bool allow = (value == types::evse_board_support::PowerOnOff::On);
    EVLOG_debug << "Allow power on: " << (allow ? "true" : "false");
    
    power_on_allowed_ = allow;
    
    if (!enable_power_output(allow)) {
        EVLOG_error << "Failed to " << (allow ? "enable" : "disable") << " power output";
        return;
    }
    
    // Publish BSP event for power state change
    types::board_support_common::BspEvent event;
    event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    event.event = allow ? types::board_support_common::BspEventEnum::PowerOn 
                        : types::board_support_common::BspEventEnum::PowerOff;
    publish_event(event);
}

void evse_board_supportImpl::handle_ac_switch_three_phases_while_charging(bool& value) {
    EVLOG_debug << "Switch to " << (value ? "3" : "1") << " phase(s) while charging";
    
    three_phase_mode_ = value;
    
    // For WhiteBeet, this might be a configuration change rather than real-time switching
    // depending on the hardware capabilities
    try {
        if (mod->whitebeet_handler) {
            // This would require extending the WhiteBeet protocol to support phase switching
            EVLOG_warning << "Phase switching during charging not implemented for WhiteBeet";
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to switch phases: " << e.what();
    }
}

void evse_board_supportImpl::handle_evse_replug(int& value) {
    EVLOG_debug << "EVSE replug simulation: " << value << " ms";
    
    // Simulate replug by briefly disabling and re-enabling
    if (enabled_) {
        set_control_pilot_mode(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
        set_control_pilot_mode(true);
        
        // Publish BSP event for replug
        types::board_support_common::BspEvent event;
        event.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
        event.event = types::board_support_common::BspEventEnum::A_F;
        publish_event(event);
    }
}

types::board_support_common::ProximityPilot evse_board_supportImpl::handle_ac_read_pp_ampacity() {
    EVLOG_debug << "Reading proximity pilot ampacity";
    
    if (!get_proximity_pilot_reading()) {
        EVLOG_warning << "Failed to read proximity pilot, returning last known value";
    }
    
    return last_pp_reading_;
}

void evse_board_supportImpl::handle_ac_set_overcurrent_limit_A(double& value) {
    EVLOG_debug << "Setting overcurrent limit: " << value << "A";
    
    overcurrent_limit_A_ = clamp_current(value);
    
    // Update WhiteBeet configuration if needed
    try {
        if (mod->whitebeet_handler) {
            // This would require extending the protocol to support dynamic current limiting
            EVLOG_debug << "Overcurrent limit set to: " << overcurrent_limit_A_ << "A";
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to set overcurrent limit: " << e.what();
    }
}

// Private methods

bool evse_board_supportImpl::set_control_pilot_mode(bool enable) {
    try {
        if (mod->whitebeet_handler) {
            if (enable) {
                return mod->whitebeet_handler->enable_control_pilot();
            } else {
                return mod->whitebeet_handler->disable_control_pilot();
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Control pilot mode change failed: " << e.what();
    }
    return false;
}

bool evse_board_supportImpl::set_duty_cycle(double percentage) {
    try {
        if (mod->whitebeet_handler) {
            return mod->whitebeet_handler->set_control_pilot_duty_cycle(percentage);
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Duty cycle setting failed: " << e.what();
    }
    return false;
}

bool evse_board_supportImpl::get_proximity_pilot_reading() {
    // For now, simulate PP reading based on configuration
    // In a real implementation, this would query the WhiteBeet module
    
    int max_current = mod->config.max_current_A;
    last_pp_reading_.ampacity = convert_pp_ampacity(max_current);
    last_pp_reading_.proximity_resistor = types::board_support_common::ProximityResistor::_1500_Ohm;
    
    return true;
}

bool evse_board_supportImpl::enable_power_output(bool enable) {
    try {
        if (mod->whitebeet_handler) {
            if (enable) {
                return mod->whitebeet_handler->enable_charging();
            } else {
                return mod->whitebeet_handler->disable_charging();
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Power output control failed: " << e.what();
    }
    return false;
}

void evse_board_supportImpl::update_capabilities() {
    // Publish hardware capabilities
    types::evse_board_support::HardwareCapabilities caps;
    caps.max_current_A_import = mod->config.max_current_A;
    caps.min_current_A_import = mod->config.min_current_A;
    caps.max_phase_count_import = mod->config.phases;
    caps.min_phase_count_import = 1;
    caps.max_current_A_export = 0; // WhiteBeet doesn't support export by default
    caps.min_current_A_export = 0;
    caps.max_phase_count_export = 0;
    caps.min_phase_count_export = 0;
    caps.supports_changing_phases_during_charging = false; // Typically not supported
    
    publish_capabilities(caps);
}

types::board_support_common::ProximityPilot evse_board_supportImpl::convert_pp_ampacity(int ampacity) {
    types::board_support_common::ProximityPilot pp;
    
    if (ampacity <= 13) {
        pp.ampacity = types::board_support_common::Ampacity::A_13;
    } else if (ampacity <= 20) {
        pp.ampacity = types::board_support_common::Ampacity::A_20;
    } else if (ampacity <= 32) {
        pp.ampacity = types::board_support_common::Ampacity::A_32;
    } else if (ampacity <= 63) {
        pp.ampacity = types::board_support_common::Ampacity::A_63_3ph_70_1ph;
    } else {
        pp.ampacity = types::board_support_common::Ampacity::A_63_3ph_70_1ph;
    }
    
    return pp;
}

double evse_board_supportImpl::clamp_current(double current_A) {
    if (current_A < mod->config.min_current_A) {
        return mod->config.min_current_A;
    } else if (current_A > mod->config.max_current_A) {
        return mod->config.max_current_A;
    }
    return current_A;
}

} // namespace board_support
} // namespace module
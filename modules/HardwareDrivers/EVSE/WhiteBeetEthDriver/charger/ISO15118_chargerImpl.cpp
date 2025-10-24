// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "ISO15118_chargerImpl.hpp"

#include <everest/logging.hpp>

namespace module {
namespace charger {

void ISO15118_chargerImpl::init() {
    EVLOG_debug << "Initializing WhiteBeet ISO15118 charger implementation";
    
    session_active_ = false;
    session_id_.clear();
    protocol_.clear();
    debug_mode_ = false;
    
    EVLOG_debug << "WhiteBeet ISO15118 charger implementation initialized";
}

void ISO15118_chargerImpl::ready() {
    EVLOG_debug << "WhiteBeet ISO15118 charger implementation ready";
}

void ISO15118_chargerImpl::handle_setup(types::iso15118_charger::EVSEID& evse_id,
                                       types::iso15118_charger::SaeJ2847BidiMode& sae_j2847_mode,
                                       bool& debug_mode) {
    EVLOG_debug << "ISO15118 setup - EVSE ID: " << evse_id.value;
    
    evse_id_ = evse_id.value;
    debug_mode_ = debug_mode;
    
    // Configure WhiteBeet module with EVSE ID
    try {
        if (mod->whitebeet_handler) {
            // The EVSE ID configuration was already done during connection setup
            EVLOG_info << "EVSE configured with ID: " << evse_id_;
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to configure EVSE ID: " << e.what();
    }
}

void ISO15118_chargerImpl::handle_set_charging_parameters(types::iso15118_charger::SetupPhysicalValues& physical_values) {
    EVLOG_debug << "Setting charging parameters";
    
    physical_values_ = physical_values;
    
    // Update WhiteBeet with new physical values if needed
    try {
        if (mod->whitebeet_handler) {
            EVLOG_debug << "Charging parameters updated";
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to update charging parameters: " << e.what();
    }
}

void ISO15118_chargerImpl::handle_session_setup(std::vector<types::iso15118_charger::PaymentOption>& payment_options,
                                               bool& supported_certificate_service,
                                               bool& central_contract_validation_allowed) {
    EVLOG_debug << "Session setup with " << payment_options.size() << " payment options";
    
    payment_options_ = payment_options;
    
    // Configure session parameters
    try {
        if (mod->whitebeet_handler) {
            EVLOG_debug << "Session setup completed";
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to setup session: " << e.what();
    }
}

void ISO15118_chargerImpl::handle_certificate_response(types::iso15118_charger::Response_Exi_Stream_Status& status,
                                                      types::iso15118_charger::CertificateActionEnum& certificate_action) {
    EVLOG_debug << "Certificate response received";
    
    // WhiteBeet handles certificate management internally
    // This implementation would forward the response to the module
}

void ISO15118_chargerImpl::handle_authorization_response(types::authorization::ValidationResult& validation_result) {
    EVLOG_debug << "Authorization response: " << types::authorization::validation_result_to_string(validation_result);
    
    // Forward authorization result to WhiteBeet
    try {
        if (mod->whitebeet_handler) {
            bool authorized = (validation_result == types::authorization::ValidationResult::Accepted);
            EVLOG_info << "Vehicle " << (authorized ? "authorized" : "not authorized");
            
            if (authorized) {
                // Publish authorization event
                types::iso15118_charger::AuthorizationStatus auth_status;
                auth_status.authorization_status = types::authorization::AuthorizationStatus::Accepted;
                auth_status.certificate_status = types::iso15118_charger::CertificateStatus::Accepted;
                publish_Authorization_Response(auth_status);
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to process authorization response: " << e.what();
    }
}

void ISO15118_chargerImpl::handle_ac_contactor_closed(bool& status) {
    EVLOG_debug << "AC contactor " << (status ? "closed" : "opened");
    
    // This information might be relevant for safety checks
}

void ISO15118_chargerImpl::handle_dlink_ready(bool& value) {
    EVLOG_debug << "Data link " << (value ? "ready" : "not ready");
    
    if (value) {
        // Data link is ready, V2G session can start
        try {
            if (mod->whitebeet_handler) {
                mod->whitebeet_handler->start_v2g_session();
            }
        } catch (const std::exception& e) {
            EVLOG_error << "Failed to start V2G session: " << e.what();
        }
    } else {
        // Data link lost, stop V2G session
        try {
            if (mod->whitebeet_handler) {
                mod->whitebeet_handler->stop_v2g_session();
            }
        } catch (const std::exception& e) {
            EVLOG_error << "Failed to stop V2G session: " << e.what();
        }
    }
}

void ISO15118_chargerImpl::handle_cable_check_finished(bool& status) {
    EVLOG_debug << "Cable check " << (status ? "passed" : "failed");
    
    // Cable check result handling
}

void ISO15118_chargerImpl::handle_receipt_is_required(bool& receipt_required) {
    EVLOG_debug << "Receipt " << (receipt_required ? "required" : "not required");
    
    // Handle receipt requirement
}

void ISO15118_chargerImpl::handle_stop_charging(bool& stop) {
    EVLOG_debug << "Stop charging: " << (stop ? "true" : "false");
    
    if (stop) {
        try {
            if (mod->whitebeet_handler) {
                mod->whitebeet_handler->disable_charging();
            }
        } catch (const std::exception& e) {
            EVLOG_error << "Failed to stop charging: " << e.what();
        }
    }
}

void ISO15118_chargerImpl::handle_update_ac_max_current(double& max_current) {
    EVLOG_debug << "Update AC max current: " << max_current << "A";
    
    // Update current limit
    physical_values_.ac_max_current_A = max_current;
}

void ISO15118_chargerImpl::handle_update_dc_maximum_limits(types::iso15118_charger::DC_EVSEMaximumLimits& maximum_limits) {
    EVLOG_debug << "Update DC maximum limits";
    
    // Update DC limits
    physical_values_.dc_limits.dc_maximum_limits = maximum_limits;
}

void ISO15118_chargerImpl::handle_update_dc_minimum_limits(types::iso15118_charger::DC_EVSEMinimumLimits& minimum_limits) {
    EVLOG_debug << "Update DC minimum limits";
    
    // Update DC limits
    physical_values_.dc_limits.dc_minimum_limits = minimum_limits;
}

void ISO15118_chargerImpl::handle_update_isolation_status(types::iso15118_charger::IsolationStatus& isolation_status) {
    EVLOG_debug << "Update isolation status";
    
    // Handle isolation status update
}

void ISO15118_chargerImpl::handle_update_dc_present_values(types::iso15118_charger::DC_EVSEPresentVoltage_Current& present_voltage_current) {
    EVLOG_debug << "Update DC present values";
    
    // Handle present values update
}

void ISO15118_chargerImpl::handle_update_meter_info(types::powermeter::Powermeter& powermeter) {
    EVLOG_debug << "Update meter info";
    
    // Forward power meter data
}

void ISO15118_chargerImpl::handle_send_error(types::iso15118_charger::EvseError& error) {
    EVLOG_error << "EVSE error: " << error.error_code;
    
    // Handle error condition
}

void ISO15118_chargerImpl::handle_reset_error() {
    EVLOG_debug << "Reset error";
    
    // Clear error conditions
}

// Private methods

void ISO15118_chargerImpl::handle_session_started(const std::string& session_id, const std::string& protocol) {
    session_active_ = true;
    session_id_ = session_id;
    protocol_ = protocol;
    
    EVLOG_info << "V2G session started - ID: " << session_id << ", Protocol: " << protocol;
    
    // Publish session event
    types::iso15118_charger::SessionEvent event;
    event.event = types::iso15118_charger::SessionEventEnum::SessionStarted;
    event.session_id = session_id;
    publish_V2G_Session_Event(event);
}

void ISO15118_chargerImpl::handle_session_stopped() {
    EVLOG_info << "V2G session stopped";
    
    // Publish session event
    types::iso15118_charger::SessionEvent event;
    event.event = types::iso15118_charger::SessionEventEnum::SessionStopped;
    event.session_id = session_id_;
    publish_V2G_Session_Event(event);
    
    session_active_ = false;
    session_id_.clear();
    protocol_.clear();
}

std::string ISO15118_chargerImpl::convert_protocol_string(const std::string& protocol) {
    // Convert WhiteBeet protocol identifier to Everest format
    if (protocol == "ISO15118_2") {
        return "ISO15118-2";
    } else if (protocol == "ISO15118_20_AC") {
        return "ISO15118-20 AC";
    } else if (protocol == "ISO15118_20_DC") {
        return "ISO15118-20 DC";
    } else if (protocol == "DIN70121") {
        return "DIN70121";
    }
    return protocol;
}

} // namespace charger
} // namespace module
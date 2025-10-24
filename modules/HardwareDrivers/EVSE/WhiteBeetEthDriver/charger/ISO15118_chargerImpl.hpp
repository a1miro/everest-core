// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef CHARGER_ISO15118_CHARGER_IMPL_HPP
#define CHARGER_ISO15118_CHARGER_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/ISO15118_charger/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace charger {

struct Conf {};

class ISO15118_chargerImpl : public ISO15118_chargerImplBase {
public:
    ISO15118_chargerImpl() = delete;
    ISO15118_chargerImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod,
                         Conf& config) :
        ISO15118_chargerImplBase(ev, "charger"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual void handle_setup(types::iso15118_charger::EVSEID& evse_id,
                             types::iso15118_charger::SaeJ2847BidiMode& sae_j2847_mode,
                             bool& debug_mode) override;
    virtual void handle_set_charging_parameters(types::iso15118_charger::SetupPhysicalValues& physical_values) override;
    virtual void handle_session_setup(std::vector<types::iso15118_charger::PaymentOption>& payment_options,
                                     bool& supported_certificate_service,
                                     bool& central_contract_validation_allowed) override;
    virtual void handle_certificate_response(types::iso15118_charger::Response_Exi_Stream_Status& status,
                                            types::iso15118_charger::CertificateActionEnum& certificate_action) override;
    virtual void handle_authorization_response(types::authorization::ValidationResult& validation_result) override;
    virtual void handle_ac_contactor_closed(bool& status) override;
    virtual void handle_dlink_ready(bool& value) override;
    virtual void handle_cable_check_finished(bool& status) override;
    virtual void handle_receipt_is_required(bool& receipt_required) override;
    virtual void handle_stop_charging(bool& stop) override;
    virtual void handle_update_ac_max_current(double& max_current) override;
    virtual void handle_update_dc_maximum_limits(types::iso15118_charger::DC_EVSEMaximumLimits& maximum_limits) override;
    virtual void handle_update_dc_minimum_limits(types::iso15118_charger::DC_EVSEMinimumLimits& minimum_limits) override;
    virtual void handle_update_isolation_status(types::iso15118_charger::IsolationStatus& isolation_status) override;
    virtual void handle_update_dc_present_values(types::iso15118_charger::DC_EVSEPresentVoltage_Current& present_voltage_current) override;
    virtual void handle_update_meter_info(types::powermeter::Powermeter& powermeter) override;
    virtual void handle_send_error(types::iso15118_charger::EvseError& error) override;
    virtual void handle_reset_error() override;

    // ev@d2d1847f-6ccc-4f0f-b2cd-5c7a9fb1fd68:v1
    // insert your protected definitions here
    // ev@d2d1847f-6ccc-4f0f-b2cd-5c7a9fb1fd68:v1

private:
    const Everest::PtrContainer<WhiteBeetEthDriver>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
    // insert your private definitions here
    
    // V2G session state
    bool session_active_;
    std::string session_id_;
    std::string protocol_;
    bool debug_mode_;
    
    // EVSE setup
    std::string evse_id_;
    types::iso15118_charger::SetupPhysicalValues physical_values_;
    std::vector<types::iso15118_charger::PaymentOption> payment_options_;
    
    // Session management
    void handle_session_started(const std::string& session_id, const std::string& protocol);
    void handle_session_stopped();
    
    // Utility methods
    std::string convert_protocol_string(const std::string& protocol);
    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace charger
} // namespace module

#endif // CHARGER_ISO15118_CHARGER_IMPL_HPP
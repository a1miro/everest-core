// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP
#define BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/evse_board_support/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace board_support {

struct Conf {};

class evse_board_supportImpl : public evse_board_supportImplBase {
public:
    evse_board_supportImpl() = delete;
    evse_board_supportImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod,
                           Conf& config) :
        evse_board_supportImplBase(ev, "board_support"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual void handle_enable(bool& value) override;
    virtual void handle_pwm_on(double& value) override;
    virtual void handle_pwm_off() override;
    virtual void handle_pwm_F() override;
    virtual void handle_allow_power_on(types::evse_board_support::PowerOnOff& value) override;
    virtual void handle_ac_switch_three_phases_while_charging(bool& value) override;
    virtual void handle_evse_replug(int& value) override;
    virtual types::board_support_common::ProximityPilot handle_ac_read_pp_ampacity() override;
    virtual void handle_ac_set_overcurrent_limit_A(double& value) override;

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
    
    // Current state tracking
    bool enabled_;
    bool power_on_allowed_;
    double current_pwm_duty_cycle_;
    bool three_phase_mode_;
    double overcurrent_limit_A_;
    types::board_support_common::ProximityPilot last_pp_reading_;
    
    // Control methods
    bool set_control_pilot_mode(bool enable);
    bool set_duty_cycle(double percentage);
    bool get_proximity_pilot_reading();
    bool enable_power_output(bool enable);
    void update_capabilities();
    
    // Utility methods
    types::board_support_common::ProximityPilot convert_pp_ampacity(int ampacity);
    double clamp_current(double current_A);
    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace board_support
} // namespace module

#endif // BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP
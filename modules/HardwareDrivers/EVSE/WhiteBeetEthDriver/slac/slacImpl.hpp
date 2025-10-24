// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef SLAC_SLAC_IMPL_HPP
#define SLAC_SLAC_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/slac/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
#include <atomic>
#include <string>
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace slac {

struct Conf {};

class slacImpl : public slacImplBase {
public:
    slacImpl() = delete;
    slacImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod, Conf& config) :
        slacImplBase(ev, "slac"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual void handle_reset(bool& enable) override;
    virtual bool handle_enter_bcd() override;
    virtual bool handle_leave_bcd() override;
    virtual bool handle_dlink_terminate() override;
    virtual bool handle_dlink_error() override;
    virtual bool handle_dlink_pause() override;

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
    
    // SLAC state management
    std::atomic<bool> slac_enabled_;
    std::atomic<bool> slac_active_;
    std::string current_state_;
    std::string matched_ev_mac_;
    
    // Control methods
    bool start_slac_process();
    bool stop_slac_process();
    bool reset_slac_state();
    void update_slac_state(const std::string& new_state);
    void handle_ev_matched(const std::string& ev_mac);
    void handle_ev_unmatched();
    
    // State validation
    bool is_valid_transition(const std::string& from_state, const std::string& to_state);
    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace slac
} // namespace module

#endif // SLAC_SLAC_IMPL_HPP
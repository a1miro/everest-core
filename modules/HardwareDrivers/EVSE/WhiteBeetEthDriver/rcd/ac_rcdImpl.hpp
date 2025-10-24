// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef RCD_AC_RCD_IMPL_HPP
#define RCD_AC_RCD_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/ac_rcd/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

namespace module {
namespace rcd {

struct Conf {};

class ac_rcdImpl : public ac_rcdImplBase {
public:
    ac_rcdImpl() = delete;
    ac_rcdImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod, Conf& config) :
        ac_rcdImplBase(ev, "rcd"), mod(mod), config(config){};

protected:
    // No command handlers for ac_rcd interface - it only publishes events

private:
    const Everest::PtrContainer<WhiteBeetEthDriver>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // Private members
    bool rcd_enabled_;
    bool last_fault_state_;
    
    void update_rcd_state(bool fault);
};

} // namespace rcd
} // namespace module

#endif // RCD_AC_RCD_IMPL_HPP
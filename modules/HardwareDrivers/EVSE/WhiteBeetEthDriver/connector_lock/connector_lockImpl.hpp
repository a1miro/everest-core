// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef CONNECTOR_LOCK_CONNECTOR_LOCK_IMPL_HPP
#define CONNECTOR_LOCK_CONNECTOR_LOCK_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/connector_lock/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

namespace module {
namespace connector_lock {

struct Conf {};

class connector_lockImpl : public connector_lockImplBase {
public:
    connector_lockImpl() = delete;
    connector_lockImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod, Conf& config) :
        connector_lockImplBase(ev, "connector_lock"), mod(mod), config(config){};

protected:
    virtual void handle_lock() override;
    virtual void handle_unlock() override;

private:
    const Everest::PtrContainer<WhiteBeetEthDriver>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // Private members
    bool lock_enabled_;
    bool current_lock_state_;
    
    void update_lock_state(bool locked);
};

} // namespace connector_lock
} // namespace module

#endif // CONNECTOR_LOCK_CONNECTOR_LOCK_IMPL_HPP
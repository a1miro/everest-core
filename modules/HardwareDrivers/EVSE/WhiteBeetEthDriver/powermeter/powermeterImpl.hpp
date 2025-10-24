// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef POWERMETER_POWERMETER_IMPL_HPP
#define POWERMETER_POWERMETER_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/powermeter/Implementation.hpp>

#include "../WhiteBeetEthDriver.hpp"

namespace module {
namespace powermeter {

struct Conf {};

class powermeterImpl : public powermeterImplBase {
public:
    powermeterImpl() = delete;
    powermeterImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<WhiteBeetEthDriver>& mod, Conf& config) :
        powermeterImplBase(ev, "powermeter"), mod(mod), config(config){};

protected:
    virtual types::powermeter::TransactionStartResponse handle_start_transaction(types::powermeter::TransactionReq& value) override;
    virtual types::powermeter::TransactionStopResponse handle_stop_transaction(std::string& transaction_id) override;

private:
    const Everest::PtrContainer<WhiteBeetEthDriver>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // Private members
    std::string current_transaction_id_;
    bool transaction_active_;
    
    std::string generate_transaction_id();
};

} // namespace powermeter
} // namespace module

#endif // POWERMETER_POWERMETER_IMPL_HPP
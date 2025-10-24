// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "ac_rcdImpl.hpp"

#include <everest/logging.hpp>

namespace module {
namespace rcd {

void ac_rcdImpl::init() {
    EVLOG_debug << "Initializing WhiteBeet AC RCD implementation";
    
    rcd_enabled_ = mod->config.has_rcd;
    last_fault_state_ = false;
    
    EVLOG_debug << "WhiteBeet AC RCD implementation initialized (enabled: " 
                << (rcd_enabled_ ? "true" : "false") << ")";
}

void ac_rcdImpl::ready() {
    EVLOG_debug << "WhiteBeet AC RCD implementation ready";
    
    if (rcd_enabled_) {
        // Initialize RCD state
        update_rcd_state(false);
    }
}

void ac_rcdImpl::update_rcd_state(bool fault) {
    if (last_fault_state_ != fault) {
        last_fault_state_ = fault;
        
        EVLOG_info << "RCD state changed: " << (fault ? "FAULT" : "OK");
        
        // Publish RCD state
        types::ac_rcd::RcdState state;
        state.rcd_current_mA = fault ? 30.0 : 0.0; // Assuming 30mA fault threshold
        publish_rcd_current_state(state);
    }
}

} // namespace rcd
} // namespace module
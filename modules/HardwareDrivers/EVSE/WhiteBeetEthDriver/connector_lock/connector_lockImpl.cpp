// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "connector_lockImpl.hpp"

#include <everest/logging.hpp>

namespace module {
namespace connector_lock {

void connector_lockImpl::init() {
    EVLOG_debug << "Initializing WhiteBeet connector lock implementation";
    
    lock_enabled_ = mod->config.has_connector_lock;
    current_lock_state_ = false;
    
    EVLOG_debug << "WhiteBeet connector lock implementation initialized (enabled: " 
                << (lock_enabled_ ? "true" : "false") << ")";
}

void connector_lockImpl::ready() {
    EVLOG_debug << "WhiteBeet connector lock implementation ready";
    
    if (lock_enabled_) {
        // Initialize lock state
        update_lock_state(false);
    }
}

void connector_lockImpl::handle_lock() {
    EVLOG_debug << "Lock connector command received";
    
    if (!lock_enabled_) {
        EVLOG_warning << "Connector lock not available on this hardware";
        return;
    }
    
    try {
        if (mod->whitebeet_handler) {
            if (mod->whitebeet_handler->lock_connector()) {
                update_lock_state(true);
                EVLOG_info << "Connector locked successfully";
            } else {
                EVLOG_error << "Failed to lock connector";
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Exception while locking connector: " << e.what();
    }
}

void connector_lockImpl::handle_unlock() {
    EVLOG_debug << "Unlock connector command received";
    
    if (!lock_enabled_) {
        EVLOG_warning << "Connector lock not available on this hardware";
        return;
    }
    
    try {
        if (mod->whitebeet_handler) {
            if (mod->whitebeet_handler->unlock_connector()) {
                update_lock_state(false);
                EVLOG_info << "Connector unlocked successfully";
            } else {
                EVLOG_error << "Failed to unlock connector";
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Exception while unlocking connector: " << e.what();
    }
}

void connector_lockImpl::update_lock_state(bool locked) {
    if (current_lock_state_ != locked) {
        current_lock_state_ = locked;
        
        EVLOG_info << "Connector lock state changed: " << (locked ? "LOCKED" : "UNLOCKED");
        
        // Publish lock state
        types::connector_lock::ConnectorLockState state;
        state.connector_lock = locked ? types::connector_lock::ConnectorLockEnum::Locked 
                                      : types::connector_lock::ConnectorLockEnum::Unlocked;
        publish_state(state);
    }
}

} // namespace connector_lock
} // namespace module
// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "slacImpl.hpp"

#include <everest/logging.hpp>

namespace module {
namespace slac {

void slacImpl::init() {
    EVLOG_debug << "Initializing WhiteBeet SLAC implementation";
    
    slac_enabled_ = false;
    slac_active_ = false;
    current_state_ = "UNMATCHED";
    matched_ev_mac_.clear();
    
    EVLOG_debug << "WhiteBeet SLAC implementation initialized";
}

void slacImpl::ready() {
    EVLOG_debug << "WhiteBeet SLAC implementation ready";
    
    // Publish initial state
    publish_state(current_state_);
    publish_dlink_ready(false);
}

void slacImpl::handle_reset(bool& enable) {
    EVLOG_debug << "SLAC reset - enable: " << (enable ? "true" : "false");
    
    slac_enabled_ = enable;
    
    if (enable) {
        // Reset SLAC state and start the process
        if (!reset_slac_state()) {
            EVLOG_error << "Failed to reset SLAC state";
            return;
        }
        
        if (!start_slac_process()) {
            EVLOG_error << "Failed to start SLAC process";
            return;
        }
        
        update_slac_state("UNMATCHED");
    } else {
        // Stop SLAC process
        if (!stop_slac_process()) {
            EVLOG_error << "Failed to stop SLAC process";
            return;
        }
        
        update_slac_state("UNMATCHED");
        publish_dlink_ready(false);
    }
}

bool slacImpl::handle_enter_bcd() {
    EVLOG_debug << "SLAC enter BCD state";
    
    if (!slac_enabled_) {
        EVLOG_warning << "SLAC not enabled, cannot enter BCD";
        return false;
    }
    
    // Check if this is a valid transition
    if (current_state_ != "UNMATCHED" && current_state_ != "MATCHING") {
        EVLOG_warning << "Invalid SLAC state transition to BCD from " << current_state_;
        return false;
    }
    
    // Start SLAC matching process
    if (!start_slac_process()) {
        EVLOG_error << "Failed to start SLAC matching";
        return false;
    }
    
    update_slac_state("MATCHING");
    return true;
}

bool slacImpl::handle_leave_bcd() {
    EVLOG_debug << "SLAC leave BCD state";
    
    if (current_state_ == "MATCHED") {
        // Terminate the data link
        return handle_dlink_terminate();
    } else {
        // Simply stop the matching process
        stop_slac_process();
        update_slac_state("UNMATCHED");
        publish_dlink_ready(false);
        return true;
    }
}

bool slacImpl::handle_dlink_terminate() {
    EVLOG_debug << "SLAC data link terminate";
    
    if (current_state_ != "MATCHED") {
        EVLOG_warning << "Data link not established, cannot terminate";
        return false;
    }
    
    // Stop SLAC and reset to unmatched state
    if (!stop_slac_process()) {
        EVLOG_error << "Failed to stop SLAC process during termination";
        return false;
    }
    
    update_slac_state("UNMATCHED");
    publish_dlink_ready(false);
    matched_ev_mac_.clear();
    
    return true;
}

bool slacImpl::handle_dlink_error() {
    EVLOG_debug << "SLAC data link error - restarting matching";
    
    // Stop current process and restart matching
    stop_slac_process();
    
    // Reset state and restart
    if (!reset_slac_state()) {
        EVLOG_error << "Failed to reset SLAC state after error";
        return false;
    }
    
    if (slac_enabled_) {
        if (!start_slac_process()) {
            EVLOG_error << "Failed to restart SLAC process after error";
            return false;
        }
        update_slac_state("MATCHING");
    } else {
        update_slac_state("UNMATCHED");
    }
    
    publish_dlink_ready(false);
    matched_ev_mac_.clear();
    
    return true;
}

bool slacImpl::handle_dlink_pause() {
    EVLOG_debug << "SLAC data link pause";
    
    if (current_state_ != "MATCHED") {
        EVLOG_warning << "Data link not established, cannot pause";
        return false;
    }
    
    // For WhiteBeet, pausing might not be directly supported
    // This would depend on the specific WhiteBeet firmware capabilities
    EVLOG_warning << "SLAC pause not implemented for WhiteBeet";
    
    // For now, just return true to indicate the command was received
    return true;
}

// Private methods

bool slacImpl::start_slac_process() {
    try {
        if (mod->whitebeet_handler) {
            bool result = mod->whitebeet_handler->start_slac();
            if (result) {
                slac_active_ = true;
                EVLOG_info << "SLAC process started successfully";
            }
            return result;
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to start SLAC process: " << e.what();
    }
    return false;
}

bool slacImpl::stop_slac_process() {
    try {
        if (mod->whitebeet_handler) {
            bool result = mod->whitebeet_handler->stop_slac();
            if (result) {
                slac_active_ = false;
                EVLOG_info << "SLAC process stopped successfully";
            }
            return result;
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to stop SLAC process: " << e.what();
    }
    return false;
}

bool slacImpl::reset_slac_state() {
    // Reset internal state
    slac_active_ = false;
    matched_ev_mac_.clear();
    
    // This could involve sending a reset command to the WhiteBeet module
    try {
        if (mod->whitebeet_handler) {
            // WhiteBeet might have a specific reset command
            EVLOG_debug << "SLAC state reset";
            return true;
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to reset SLAC state: " << e.what();
    }
    return false;
}

void slacImpl::update_slac_state(const std::string& new_state) {
    if (current_state_ != new_state) {
        EVLOG_info << "SLAC state change: " << current_state_ << " -> " << new_state;
        current_state_ = new_state;
        publish_state(current_state_);
        
        // Update data link ready status
        bool dlink_ready = (current_state_ == "MATCHED");
        publish_dlink_ready(dlink_ready);
    }
}

void slacImpl::handle_ev_matched(const std::string& ev_mac) {
    EVLOG_info << "EV matched via SLAC: " << ev_mac;
    
    matched_ev_mac_ = ev_mac;
    update_slac_state("MATCHED");
    
    // Publish the EV MAC address
    publish_ev_mac_address(ev_mac);
    publish_dlink_ready(true);
}

void slacImpl::handle_ev_unmatched() {
    EVLOG_info << "EV unmatched from SLAC";
    
    matched_ev_mac_.clear();
    update_slac_state("UNMATCHED");
    publish_dlink_ready(false);
}

bool slacImpl::is_valid_transition(const std::string& from_state, const std::string& to_state) {
    // Define valid SLAC state transitions
    if (from_state == "UNMATCHED") {
        return (to_state == "MATCHING");
    } else if (from_state == "MATCHING") {
        return (to_state == "MATCHED" || to_state == "UNMATCHED");
    } else if (from_state == "MATCHED") {
        return (to_state == "UNMATCHED" || to_state == "MATCHING");
    }
    
    return false;
}

} // namespace slac
} // namespace module
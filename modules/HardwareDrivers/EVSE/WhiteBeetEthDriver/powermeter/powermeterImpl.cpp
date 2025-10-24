// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "powermeterImpl.hpp"

#include <everest/logging.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace module {
namespace powermeter {

void powermeterImpl::init() {
    EVLOG_debug << "Initializing WhiteBeet powermeter implementation";
    
    transaction_active_ = false;
    current_transaction_id_.clear();
    
    EVLOG_debug << "WhiteBeet powermeter implementation initialized";
}

void powermeterImpl::ready() {
    EVLOG_debug << "WhiteBeet powermeter implementation ready";
}

types::powermeter::TransactionStartResponse powermeterImpl::handle_start_transaction(types::powermeter::TransactionReq& value) {
    EVLOG_debug << "Starting power meter transaction";
    
    types::powermeter::TransactionStartResponse response;
    
    if (transaction_active_) {
        EVLOG_warning << "Transaction already active, stopping previous transaction";
        // Force stop previous transaction
        types::powermeter::TransactionStopResponse stop_response = handle_stop_transaction(current_transaction_id_);
    }
    
    // Generate new transaction ID
    current_transaction_id_ = generate_transaction_id();
    transaction_active_ = true;
    
    response.status = types::powermeter::TransactionRequestStatus::OK;
    response.transaction_id = current_transaction_id_;
    response.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    
    // Get initial power measurement
    try {
        if (mod->whitebeet_handler) {
            double voltage, current, power, energy;
            if (mod->whitebeet_handler->get_power_measurement(voltage, current, power, energy)) {
                response.energy_Wh_import.total = energy;
                EVLOG_info << "Transaction started with ID: " << current_transaction_id_ 
                          << ", initial energy: " << energy << " Wh";
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to get initial power measurement: " << e.what();
        response.energy_Wh_import.total = 0.0;
    }
    
    return response;
}

types::powermeter::TransactionStopResponse powermeterImpl::handle_stop_transaction(std::string& transaction_id) {
    EVLOG_debug << "Stopping power meter transaction: " << transaction_id;
    
    types::powermeter::TransactionStopResponse response;
    
    if (!transaction_active_ || current_transaction_id_ != transaction_id) {
        EVLOG_warning << "No active transaction or transaction ID mismatch";
        response.status = types::powermeter::TransactionRequestStatus::UNEXPECTED_ERROR;
        return response;
    }
    
    response.status = types::powermeter::TransactionRequestStatus::OK;
    response.timestamp = Everest::Date::to_rfc3339(date::utc_clock::now());
    
    // Get final power measurement
    try {
        if (mod->whitebeet_handler) {
            double voltage, current, power, energy;
            if (mod->whitebeet_handler->get_power_measurement(voltage, current, power, energy)) {
                response.energy_Wh_import.total = energy;
                EVLOG_info << "Transaction stopped with ID: " << transaction_id 
                          << ", final energy: " << energy << " Wh";
            }
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Failed to get final power measurement: " << e.what();
        response.energy_Wh_import.total = 0.0;
    }
    
    transaction_active_ = false;
    current_transaction_id_.clear();
    
    return response;
}

std::string powermeterImpl::generate_transaction_id() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << "WB_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
        << "_" << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

} // namespace powermeter
} // namespace module
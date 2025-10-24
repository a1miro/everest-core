// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef WHITEBEET_PROTOCOL_HPP
#define WHITEBEET_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace whitebeet_comms {

// Protocol constants based on FreeV2G and WhiteBeet documentation
namespace protocol {
    constexpr uint16_t MAGIC_HEADER = 0xABCD;
    constexpr uint8_t PROTOCOL_VERSION = 0x01;
    constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;
    constexpr uint16_t HEADER_SIZE = 8;
}

// Message types based on WhiteBeet HCI (Host Control Interface)
enum class MessageType : uint16_t {
    // System messages
    PING = 0x0001,
    PONG = 0x0002,
    RESET = 0x0003,
    CONFIG = 0x0004,
    CONFIG_ACK = 0x0005,
    STATUS_REQUEST = 0x0006,
    STATUS_RESPONSE = 0x0007,
    ERROR = 0x0008,
    
    // Control Pilot messages
    CP_SET_MODE = 0x0010,
    CP_SET_DUTY_CYCLE = 0x0011,
    CP_GET_STATE = 0x0012,
    CP_STATE_CHANGED = 0x0013,
    
    // SLAC messages
    SLAC_START = 0x0020,
    SLAC_STOP = 0x0021,
    SLAC_RESET = 0x0022,
    SLAC_STATUS = 0x0023,
    SLAC_MATCHED = 0x0024,
    SLAC_UNMATCHED = 0x0025,
    SLAC_MATCHING = 0x0026,
    
    // V2G messages
    V2G_START = 0x0030,
    V2G_STOP = 0x0031,
    V2G_SESSION_SETUP = 0x0032,
    V2G_SESSION_STARTED = 0x0033,
    V2G_SESSION_STOPPED = 0x0034,
    V2G_AUTHORIZATION_REQ = 0x0035,
    V2G_AUTHORIZATION_RES = 0x0036,
    V2G_CHARGE_PARAMETER_DISCOVERY = 0x0037,
    V2G_CHARGING_STATUS = 0x0038,
    V2G_POWER_DELIVERY = 0x0039,
    
    // Power measurement messages
    POWER_MEASUREMENT = 0x0040,
    POWER_LIMITS = 0x0041,
    ENERGY_TRANSFER = 0x0042,
    
    // Hardware control messages
    RCD_STATUS = 0x0050,
    RCD_FAULT = 0x0051,
    CONNECTOR_LOCK = 0x0052,
    CONNECTOR_UNLOCK = 0x0053,
    CONNECTOR_LOCK_STATUS = 0x0054,
    
    // Charging control
    CHARGING_ENABLE = 0x0060,
    CHARGING_DISABLE = 0x0061,
    CHARGING_STARTED = 0x0062,
    CHARGING_STOPPED = 0x0063,
    CHARGING_PARAMETERS = 0x0064
};

// Control Pilot states
enum class ControlPilotState : uint8_t {
    STATE_A = 0x01,  // Not connected
    STATE_B = 0x02,  // Connected, not ready
    STATE_C = 0x03,  // Connected, ready, ventilation not required
    STATE_D = 0x04,  // Connected, ready, ventilation required
    STATE_E = 0x05,  // No power available (shutdown)
    STATE_F = 0x06,  // Error/fault
    UNKNOWN = 0xFF
};

// SLAC states
enum class SlacState : uint8_t {
    UNMATCHED = 0x01,
    MATCHING = 0x02,
    MATCHED = 0x03,
    ERROR = 0xFF
};

// V2G protocols
enum class V2GProtocol : uint8_t {
    ISO15118_2 = 0x01,
    ISO15118_20_AC = 0x02,
    ISO15118_20_DC = 0x03,
    DIN70121 = 0x04,
    UNKNOWN = 0xFF
};

// Error codes
enum class ErrorCode : uint16_t {
    NO_ERROR = 0x0000,
    GENERIC_ERROR = 0x0001,
    COMMUNICATION_ERROR = 0x0002,
    INVALID_MESSAGE = 0x0003,
    INVALID_PARAMETER = 0x0004,
    SLAC_ERROR = 0x0010,
    V2G_ERROR = 0x0020,
    CHARGING_ERROR = 0x0030,
    RCD_ERROR = 0x0040,
    CONNECTOR_LOCK_ERROR = 0x0050,
    POWER_ERROR = 0x0060
};

// Message header structure
struct MessageHeader {
    uint16_t magic;          // Magic number (0xABCD)
    uint8_t version;         // Protocol version
    uint8_t reserved;        // Reserved byte
    uint16_t message_type;   // Message type
    uint16_t payload_length; // Length of payload
    
    MessageHeader() : magic(protocol::MAGIC_HEADER), version(protocol::PROTOCOL_VERSION), 
                     reserved(0), message_type(0), payload_length(0) {}
};

// Generic message structure
struct WhiteBeetMessage {
    MessageHeader header;
    std::vector<uint8_t> payload;
    uint16_t message_id;     // For request/response correlation
    
    WhiteBeetMessage() : message_id(0) {}
    
    // Convenience methods
    void set_message_type(MessageType type) {
        header.message_type = static_cast<uint16_t>(type);
    }
    
    MessageType get_message_type() const {
        return static_cast<MessageType>(header.message_type);
    }
    
    void set_payload(const std::vector<uint8_t>& data) {
        payload = data;
        header.payload_length = static_cast<uint16_t>(payload.size());
    }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

// Specific message payloads
struct ConfigPayload {
    char evse_id[32];
    uint16_t max_current_mA;     // mA for precision
    uint16_t min_current_mA;     // mA for precision
    uint16_t max_voltage_V;
    uint16_t min_voltage_V;
    uint32_t max_power_W;
    uint8_t phases;
    uint8_t connector_type;      // 0=AC, 1=DC
    uint8_t features;            // Bit field for capabilities
    uint8_t reserved[5];
    
    ConfigPayload() {
        memset(this, 0, sizeof(ConfigPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct ControlPilotPayload {
    uint8_t mode;               // 0=disable, 1=enable
    uint16_t duty_cycle_permille; // Duty cycle in permille (0-1000)
    uint8_t state;              // Current CP state
    uint8_t reserved[4];
    
    ControlPilotPayload() {
        memset(this, 0, sizeof(ControlPilotPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct SlacPayload {
    uint8_t state;              // SLAC state
    uint8_t ev_mac[6];          // EV MAC address
    uint8_t nid[7];             // Network ID
    uint8_t reserved[2];
    
    SlacPayload() {
        memset(this, 0, sizeof(SlacPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct V2GSessionPayload {
    char session_id[16];
    uint8_t protocol;           // V2G protocol used
    uint8_t state;              // Session state
    uint16_t evcc_id;           // EVCC identifier
    uint32_t timestamp;         // Session timestamp
    uint8_t reserved[4];
    
    V2GSessionPayload() {
        memset(this, 0, sizeof(V2GSessionPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct PowerMeasurementPayload {
    uint32_t voltage_mV;        // Voltage in mV
    uint32_t current_mA;        // Current in mA
    uint32_t power_mW;          // Power in mW
    uint64_t energy_mWh;        // Energy in mWh
    uint32_t timestamp;         // Measurement timestamp
    uint8_t reserved[4];
    
    PowerMeasurementPayload() {
        memset(this, 0, sizeof(PowerMeasurementPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct StatusPayload {
    uint8_t component;          // Component identifier
    uint8_t state;              // Component state
    uint16_t error_code;        // Error code if any
    uint32_t data;              // Component-specific data
    uint8_t reserved[8];
    
    StatusPayload() {
        memset(this, 0, sizeof(StatusPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

struct ErrorPayload {
    uint16_t error_code;
    char error_message[64];
    uint32_t timestamp;
    uint8_t reserved[4];
    
    ErrorPayload() {
        memset(this, 0, sizeof(ErrorPayload));
    }
    
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};

// Utility functions for message handling
namespace utils {
    uint16_t calculate_checksum(const std::vector<uint8_t>& data);
    bool validate_checksum(const std::vector<uint8_t>& data, uint16_t expected_checksum);
    std::string message_type_to_string(MessageType type);
    std::string control_pilot_state_to_string(ControlPilotState state);
    std::string slac_state_to_string(SlacState state);
    std::string v2g_protocol_to_string(V2GProtocol protocol);
    std::string error_code_to_string(ErrorCode code);
    
    // Message factory functions
    WhiteBeetMessage create_ping_message(uint16_t message_id);
    WhiteBeetMessage create_config_message(uint16_t message_id, const ConfigPayload& config);
    WhiteBeetMessage create_cp_control_message(uint16_t message_id, bool enable, double duty_cycle_percent);
    WhiteBeetMessage create_slac_control_message(uint16_t message_id, bool start);
    WhiteBeetMessage create_status_request_message(uint16_t message_id, uint8_t component);
    WhiteBeetMessage create_error_message(uint16_t message_id, ErrorCode code, const std::string& message);
}

} // namespace whitebeet_comms

#endif // WHITEBEET_PROTOCOL_HPP
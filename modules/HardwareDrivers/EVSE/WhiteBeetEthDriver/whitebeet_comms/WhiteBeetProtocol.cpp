#include "WhiteBeetProtocol.hpp"
#include <cstring>
#include <everest/logging.hpp>

namespace whitebeet_comms {

WhiteBeetProtocol::WhiteBeetProtocol() {
    sequence_number = 0;
}

WhiteBeetProtocol::~WhiteBeetProtocol() {
}

std::vector<uint8_t> WhiteBeetProtocol::create_message(MessageType type, 
                                                       const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> message;
    
    // Header structure:
    // [0-1]: Message length (16-bit, little endian)
    // [2]: Message type
    // [3]: Sequence number
    // [4-7]: Reserved (4 bytes)
    // [8+]: Payload
    
    uint16_t total_length = 8 + payload.size();
    
    message.reserve(total_length);
    
    // Message length (little endian)
    message.push_back(total_length & 0xFF);
    message.push_back((total_length >> 8) & 0xFF);
    
    // Message type
    message.push_back(static_cast<uint8_t>(type));
    
    // Sequence number
    message.push_back(sequence_number++);
    
    // Reserved bytes
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    
    // Payload
    message.insert(message.end(), payload.begin(), payload.end());
    
    return message;
}

bool WhiteBeetProtocol::parse_message(const std::vector<uint8_t>& data,
                                      MessageType& type,
                                      uint8_t& seq_num,
                                      std::vector<uint8_t>& payload) {
    if (data.size() < 8) {
        EVLOG_error << "WhiteBeet message too short: " << data.size() << " bytes";
        return false;
    }
    
    // Extract length
    uint16_t length = data[0] | (static_cast<uint16_t>(data[1]) << 8);
    
    if (data.size() != length) {
        EVLOG_error << "WhiteBeet message length mismatch. Expected: " << length 
                   << ", got: " << data.size();
        return false;
    }
    
    // Extract type and sequence number
    type = static_cast<MessageType>(data[2]);
    seq_num = data[3];
    
    // Extract payload
    if (data.size() > 8) {
        payload.assign(data.begin() + 8, data.end());
    } else {
        payload.clear();
    }
    
    return true;
}

std::vector<uint8_t> WhiteBeetProtocol::create_slac_request(SlacRequestType req_type, 
                                                            const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    
    // SLAC request structure:
    // [0]: Request type
    // [1]: Request length
    // [2+]: Request data
    
    payload.push_back(static_cast<uint8_t>(req_type));
    payload.push_back(static_cast<uint8_t>(data.size()));
    payload.insert(payload.end(), data.begin(), data.end());
    
    return create_message(MessageType::SLAC_REQUEST, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_slac_response(SlacResponseType resp_type,
                                                             const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(resp_type));
    payload.push_back(static_cast<uint8_t>(data.size()));
    payload.insert(payload.end(), data.begin(), data.end());
    
    return create_message(MessageType::SLAC_RESPONSE, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_iso15118_request(Iso15118RequestType req_type,
                                                                const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(req_type));
    payload.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    payload.push_back(static_cast<uint8_t>((data.size() >> 8) & 0xFF));
    payload.insert(payload.end(), data.begin(), data.end());
    
    return create_message(MessageType::ISO15118_REQUEST, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_iso15118_response(Iso15118ResponseType resp_type,
                                                                 const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(resp_type));
    payload.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    payload.push_back(static_cast<uint8_t>((data.size() >> 8) & 0xFF));
    payload.insert(payload.end(), data.begin(), data.end());
    
    return create_message(MessageType::ISO15118_RESPONSE, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_board_support_command(BoardSupportCommand cmd,
                                                                     const std::vector<uint8_t>& params) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(cmd));
    payload.insert(payload.end(), params.begin(), params.end());
    
    return create_message(MessageType::BOARD_SUPPORT, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_powermeter_request(PowermeterCommand cmd) {
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(cmd));
    
    return create_message(MessageType::POWERMETER_REQUEST, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_rcd_command(RcdCommand cmd, bool enabled) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(cmd));
    payload.push_back(enabled ? 0x01 : 0x00);
    
    return create_message(MessageType::RCD_COMMAND, payload);
}

std::vector<uint8_t> WhiteBeetProtocol::create_connector_lock_command(ConnectorLockCommand cmd, bool lock) {
    std::vector<uint8_t> payload;
    
    payload.push_back(static_cast<uint8_t>(cmd));
    payload.push_back(lock ? 0x01 : 0x00);
    
    return create_message(MessageType::CONNECTOR_LOCK, payload);
}

bool WhiteBeetProtocol::parse_slac_message(const std::vector<uint8_t>& payload,
                                           SlacRequestType& req_type,
                                           std::vector<uint8_t>& data) {
    if (payload.size() < 2) {
        return false;
    }
    
    req_type = static_cast<SlacRequestType>(payload[0]);
    uint8_t data_length = payload[1];
    
    if (payload.size() < 2 + data_length) {
        return false;
    }
    
    data.assign(payload.begin() + 2, payload.begin() + 2 + data_length);
    return true;
}

bool WhiteBeetProtocol::parse_iso15118_message(const std::vector<uint8_t>& payload,
                                               Iso15118RequestType& req_type,
                                               std::vector<uint8_t>& data) {
    if (payload.size() < 3) {
        return false;
    }
    
    req_type = static_cast<Iso15118RequestType>(payload[0]);
    uint16_t data_length = payload[1] | (static_cast<uint16_t>(payload[2]) << 8);
    
    if (payload.size() < 3 + data_length) {
        return false;
    }
    
    data.assign(payload.begin() + 3, payload.begin() + 3 + data_length);
    return true;
}

bool WhiteBeetProtocol::parse_board_support_message(const std::vector<uint8_t>& payload,
                                                    BoardSupportCommand& cmd,
                                                    std::vector<uint8_t>& params) {
    if (payload.empty()) {
        return false;
    }
    
    cmd = static_cast<BoardSupportCommand>(payload[0]);
    
    if (payload.size() > 1) {
        params.assign(payload.begin() + 1, payload.end());
    } else {
        params.clear();
    }
    
    return true;
}

std::string WhiteBeetProtocol::message_type_to_string(MessageType type) {
    switch (type) {
        case MessageType::SLAC_REQUEST: return "SLAC_REQUEST";
        case MessageType::SLAC_RESPONSE: return "SLAC_RESPONSE";
        case MessageType::ISO15118_REQUEST: return "ISO15118_REQUEST";
        case MessageType::ISO15118_RESPONSE: return "ISO15118_RESPONSE";
        case MessageType::BOARD_SUPPORT: return "BOARD_SUPPORT";
        case MessageType::POWERMETER_REQUEST: return "POWERMETER_REQUEST";
        case MessageType::POWERMETER_RESPONSE: return "POWERMETER_RESPONSE";
        case MessageType::RCD_COMMAND: return "RCD_COMMAND";
        case MessageType::RCD_STATUS: return "RCD_STATUS";
        case MessageType::CONNECTOR_LOCK: return "CONNECTOR_LOCK";
        case MessageType::STATUS_UPDATE: return "STATUS_UPDATE";
        case MessageType::ERROR_INDICATION: return "ERROR_INDICATION";
        default: return "UNKNOWN";
    }
}

} // namespace whitebeet_comms
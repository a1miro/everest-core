#!/usr/bin/env python3
"""
WhiteBeet Ethernet Communication Module

This module handles communication with the 8devices WHITE-beet-EI ISO15118 EVSE module
via Ethernet using TCP/IP protocol.

Based on the FreeV2G project: https://github.com/Sevenstax/FreeV2G
"""

import socket
import struct
import threading
import time
import logging
import json
from typing import Dict, List, Optional, Callable, Any
from enum import Enum
from dataclasses import dataclass

logger = logging.getLogger(__name__)

class MessageType(Enum):
    """WhiteBeet HCI message types"""
    SLAC_REQUEST = 0x01
    SLAC_RESPONSE = 0x02
    ISO15118_REQUEST = 0x03
    ISO15118_RESPONSE = 0x04
    BOARD_SUPPORT = 0x05
    POWERMETER_REQUEST = 0x06
    POWERMETER_RESPONSE = 0x07
    RCD_COMMAND = 0x08
    RCD_STATUS = 0x09
    CONNECTOR_LOCK = 0x0A
    STATUS_UPDATE = 0x0B
    ERROR_INDICATION = 0x0C

class SlacRequestType(Enum):
    """SLAC request types"""
    START_MATCHING = 0x01
    STOP_MATCHING = 0x02
    GET_STATUS = 0x03
    RESET = 0x04

class SlacResponseType(Enum):
    """SLAC response types"""
    MATCHING_STARTED = 0x01
    MATCHING_STOPPED = 0x02
    MATCHING_SUCCESS = 0x03
    MATCHING_FAILED = 0x04
    STATUS_REPORT = 0x05

class Iso15118RequestType(Enum):
    """ISO 15118 request types"""
    START_SESSION = 0x01
    STOP_SESSION = 0x02
    PAUSE_SESSION = 0x03
    RESUME_SESSION = 0x04
    GET_STATUS = 0x05

class BoardSupportCommand(Enum):
    """Board support commands"""
    ENABLE_RELAY = 0x01
    DISABLE_RELAY = 0x02
    READ_CP_VOLTAGE = 0x03
    SET_CP_VOLTAGE = 0x04
    READ_PROXIMITY = 0x05
    GET_STATUS = 0x06

class PowermeterCommand(Enum):
    """Powermeter commands"""
    GET_MEASUREMENT = 0x01
    START_MEASUREMENT = 0x02
    STOP_MEASUREMENT = 0x03

class RcdCommand(Enum):
    """RCD commands"""
    ENABLE = 0x01
    DISABLE = 0x02
    GET_STATUS = 0x03
    TEST = 0x04

class ConnectorLockCommand(Enum):
    """Connector lock commands"""
    LOCK = 0x01
    UNLOCK = 0x02
    GET_STATUS = 0x03

@dataclass
class WhiteBeetMessage:
    """WhiteBeet HCI message structure"""
    message_type: MessageType
    sequence_number: int
    payload: bytes

@dataclass
class PowerMeterData:
    """Power meter measurement data"""
    voltage_l1: float
    voltage_l2: float
    voltage_l3: float
    current_l1: float
    current_l2: float
    current_l3: float
    power_active: float
    power_reactive: float
    energy_active_import: float
    energy_active_export: float
    frequency: float

@dataclass
class BoardStatus:
    """Board support status"""
    relay_enabled: bool
    cp_voltage: float
    proximity_detected: bool
    temperature: float

class WhiteBeetEthernet:
    """
    WhiteBeet Ethernet communication handler
    
    This class manages TCP/IP communication with the WhiteBeet module,
    implementing the WhiteBeet HCI (Host Control Interface) protocol.
    """
    
    def __init__(self, ip_address: str, port: int, timeout: float = 5.0):
        self.ip_address = ip_address
        self.port = port
        self.timeout = timeout
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.running = False
        self.sequence_number = 0
        self.receive_thread: Optional[threading.Thread] = None
        self.message_callbacks: Dict[MessageType, Callable] = {}
        self.lock = threading.Lock()
        
    def connect(self) -> bool:
        """Connect to WhiteBeet module"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.ip_address, self.port))
            self.connected = True
            logger.info(f"Connected to WhiteBeet at {self.ip_address}:{self.port}")
            return True
        except Exception as e:
            logger.error(f"Failed to connect to WhiteBeet: {e}")
            self.socket = None
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from WhiteBeet module"""
        self.running = False
        self.connected = False
        
        if self.receive_thread and self.receive_thread.is_alive():
            self.receive_thread.join(timeout=2.0)
        
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            
        logger.info("Disconnected from WhiteBeet")
    
    def start_receive_thread(self):
        """Start the message receive thread"""
        if self.running:
            return
            
        self.running = True
        self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.receive_thread.start()
        logger.info("WhiteBeet receive thread started")
    
    def stop_receive_thread(self):
        """Stop the message receive thread"""
        self.running = False
        if self.receive_thread and self.receive_thread.is_alive():
            self.receive_thread.join(timeout=2.0)
        logger.info("WhiteBeet receive thread stopped")
    
    def register_callback(self, message_type: MessageType, callback: Callable):
        """Register a callback for specific message types"""
        self.message_callbacks[message_type] = callback
    
    def send_message(self, msg_type: MessageType, payload: bytes = b'') -> bool:
        """Send a message to WhiteBeet module"""
        if not self.connected or not self.socket:
            logger.error("Not connected to WhiteBeet")
            return False
        
        try:
            with self.lock:
                # Create message header
                # Format: [length:2][type:1][seq:1][reserved:4][payload:...]
                message_length = 8 + len(payload)
                seq_num = self.sequence_number
                self.sequence_number = (self.sequence_number + 1) % 256
                
                header = struct.pack('<HBB4x', message_length, msg_type.value, seq_num)
                message = header + payload
                
                self.socket.send(message)
                logger.debug(f"Sent message type {msg_type.name}, seq {seq_num}, payload {len(payload)} bytes")
                return True
                
        except Exception as e:
            logger.error(f"Failed to send message: {e}")
            self.disconnect()
            return False
    
    def _receive_loop(self):
        """Main receive loop running in separate thread"""
        while self.running and self.connected:
            try:
                # Receive header (8 bytes)
                header_data = self._receive_exact(8)
                if not header_data:
                    continue
                
                # Parse header
                length, msg_type_val, seq_num = struct.unpack('<HBBxxxx', header_data)
                
                # Receive payload if any
                payload_length = length - 8
                payload = b''
                if payload_length > 0:
                    payload = self._receive_exact(payload_length)
                    if not payload:
                        continue
                
                # Create message object
                try:
                    msg_type = MessageType(msg_type_val)
                except ValueError:
                    logger.warning(f"Unknown message type: {msg_type_val}")
                    continue
                
                message = WhiteBeetMessage(msg_type, seq_num, payload)
                logger.debug(f"Received message: {msg_type.name}, seq {seq_num}, payload {len(payload)} bytes")
                
                # Call registered callback
                if msg_type in self.message_callbacks:
                    try:
                        self.message_callbacks[msg_type](message)
                    except Exception as e:
                        logger.error(f"Error in message callback: {e}")
                
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    logger.error(f"Error in receive loop: {e}")
                    self.disconnect()
                break
    
    def _receive_exact(self, length: int) -> Optional[bytes]:
        """Receive exact number of bytes"""
        if not self.socket:
            return None
            
        data = b''
        while len(data) < length:
            try:
                chunk = self.socket.recv(length - len(data))
                if not chunk:
                    logger.error("Connection closed by peer")
                    self.disconnect()
                    return None
                data += chunk
            except socket.timeout:
                if not self.running:
                    return None
                continue
            except Exception as e:
                logger.error(f"Receive error: {e}")
                return None
        return data
    
    # High-level command methods
    
    def send_slac_request(self, req_type: SlacRequestType, data: bytes = b'') -> bool:
        """Send SLAC request"""
        payload = struct.pack('BB', req_type.value, len(data)) + data
        return self.send_message(MessageType.SLAC_REQUEST, payload)
    
    def send_iso15118_request(self, req_type: Iso15118RequestType, data: bytes = b'') -> bool:
        """Send ISO 15118 request"""
        payload = struct.pack('BH', req_type.value, len(data)) + data
        return self.send_message(MessageType.ISO15118_REQUEST, payload)
    
    def send_board_support_command(self, cmd: BoardSupportCommand, params: bytes = b'') -> bool:
        """Send board support command"""
        payload = struct.pack('B', cmd.value) + params
        return self.send_message(MessageType.BOARD_SUPPORT, payload)
    
    def send_powermeter_request(self, cmd: PowermeterCommand) -> bool:
        """Send powermeter request"""
        payload = struct.pack('B', cmd.value)
        return self.send_message(MessageType.POWERMETER_REQUEST, payload)
    
    def send_rcd_command(self, cmd: RcdCommand, enabled: bool = False) -> bool:
        """Send RCD command"""
        payload = struct.pack('BB', cmd.value, 1 if enabled else 0)
        return self.send_message(MessageType.RCD_COMMAND, payload)
    
    def send_connector_lock_command(self, cmd: ConnectorLockCommand, lock: bool = False) -> bool:
        """Send connector lock command"""
        payload = struct.pack('BB', cmd.value, 1 if lock else 0)
        return self.send_message(MessageType.CONNECTOR_LOCK, payload)

class WhiteBeetProtocol:
    """
    High-level WhiteBeet protocol implementation
    
    This class provides a high-level interface for interacting with WhiteBeet
    functionality, abstracting away the low-level message handling.
    """
    
    def __init__(self, ethernet: WhiteBeetEthernet):
        self.ethernet = ethernet
        self.response_handlers: Dict[MessageType, Callable] = {}
        self.latest_responses: Dict[MessageType, WhiteBeetMessage] = {}
        self.response_events: Dict[MessageType, threading.Event] = {}
        
        # Register message handlers
        self.ethernet.register_callback(MessageType.SLAC_RESPONSE, self._handle_slac_response)
        self.ethernet.register_callback(MessageType.ISO15118_RESPONSE, self._handle_iso15118_response)
        self.ethernet.register_callback(MessageType.POWERMETER_RESPONSE, self._handle_powermeter_response)
        self.ethernet.register_callback(MessageType.RCD_STATUS, self._handle_rcd_status)
        self.ethernet.register_callback(MessageType.STATUS_UPDATE, self._handle_status_update)
        self.ethernet.register_callback(MessageType.ERROR_INDICATION, self._handle_error_indication)
    
    def _handle_slac_response(self, message: WhiteBeetMessage):
        """Handle SLAC response messages"""
        logger.debug(f"SLAC response received: {message.payload.hex()}")
        self.latest_responses[MessageType.SLAC_RESPONSE] = message
        if MessageType.SLAC_RESPONSE in self.response_events:
            self.response_events[MessageType.SLAC_RESPONSE].set()
    
    def _handle_iso15118_response(self, message: WhiteBeetMessage):
        """Handle ISO 15118 response messages"""
        logger.debug(f"ISO 15118 response received: {message.payload.hex()}")
        self.latest_responses[MessageType.ISO15118_RESPONSE] = message
        if MessageType.ISO15118_RESPONSE in self.response_events:
            self.response_events[MessageType.ISO15118_RESPONSE].set()
    
    def _handle_powermeter_response(self, message: WhiteBeetMessage):
        """Handle powermeter response messages"""
        logger.debug(f"Powermeter response received: {message.payload.hex()}")
        self.latest_responses[MessageType.POWERMETER_RESPONSE] = message
        if MessageType.POWERMETER_RESPONSE in self.response_events:
            self.response_events[MessageType.POWERMETER_RESPONSE].set()
    
    def _handle_rcd_status(self, message: WhiteBeetMessage):
        """Handle RCD status messages"""
        logger.debug(f"RCD status received: {message.payload.hex()}")
        self.latest_responses[MessageType.RCD_STATUS] = message
    
    def _handle_status_update(self, message: WhiteBeetMessage):
        """Handle general status update messages"""
        logger.debug(f"Status update received: {message.payload.hex()}")
    
    def _handle_error_indication(self, message: WhiteBeetMessage):
        """Handle error indication messages"""
        logger.warning(f"Error indication received: {message.payload.hex()}")
    
    def wait_for_response(self, msg_type: MessageType, timeout: float = 5.0) -> Optional[WhiteBeetMessage]:
        """Wait for a specific response message"""
        event = threading.Event()
        self.response_events[msg_type] = event
        
        if event.wait(timeout):
            return self.latest_responses.get(msg_type)
        else:
            logger.warning(f"Timeout waiting for {msg_type.name} response")
            return None
    
    # High-level command methods with responses
    
    def start_slac_matching(self, timeout: float = 5.0) -> bool:
        """Start SLAC matching process"""
        if not self.ethernet.send_slac_request(SlacRequestType.START_MATCHING):
            return False
        
        response = self.wait_for_response(MessageType.SLAC_RESPONSE, timeout)
        if response and len(response.payload) >= 1:
            resp_type = SlacResponseType(response.payload[0])
            return resp_type == SlacResponseType.MATCHING_STARTED
        return False
    
    def stop_slac_matching(self, timeout: float = 5.0) -> bool:
        """Stop SLAC matching process"""
        if not self.ethernet.send_slac_request(SlacRequestType.STOP_MATCHING):
            return False
        
        response = self.wait_for_response(MessageType.SLAC_RESPONSE, timeout)
        if response and len(response.payload) >= 1:
            resp_type = SlacResponseType(response.payload[0])
            return resp_type == SlacResponseType.MATCHING_STOPPED
        return False
    
    def get_powermeter_data(self, timeout: float = 5.0) -> Optional[PowerMeterData]:
        """Get current powermeter measurements"""
        if not self.ethernet.send_powermeter_request(PowermeterCommand.GET_MEASUREMENT):
            return None
        
        response = self.wait_for_response(MessageType.POWERMETER_RESPONSE, timeout)
        if response and len(response.payload) >= 44:  # 11 floats * 4 bytes
            # Parse power meter data
            values = struct.unpack('<11f', response.payload[:44])
            return PowerMeterData(*values)
        return None
    
    def enable_relay(self, timeout: float = 5.0) -> bool:
        """Enable the main relay"""
        return self.ethernet.send_board_support_command(BoardSupportCommand.ENABLE_RELAY)
    
    def disable_relay(self, timeout: float = 5.0) -> bool:
        """Disable the main relay"""
        return self.ethernet.send_board_support_command(BoardSupportCommand.DISABLE_RELAY)
    
    def lock_connector(self, timeout: float = 5.0) -> bool:
        """Lock the connector"""
        return self.ethernet.send_connector_lock_command(ConnectorLockCommand.LOCK, True)
    
    def unlock_connector(self, timeout: float = 5.0) -> bool:
        """Unlock the connector"""
        return self.ethernet.send_connector_lock_command(ConnectorLockCommand.UNLOCK, False)
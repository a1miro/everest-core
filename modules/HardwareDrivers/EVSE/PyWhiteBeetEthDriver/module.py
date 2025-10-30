#!/usr/bin/env python3
"""
PyWhiteBeetEthDriver - Python EVerest Module

This module implements an EVerest hardware driver for the 8devices WHITE-beet-EI 
ISO15118 EVSE module using Python and the WhiteBeet HCI protocol.

Based on FreeV2G project: https://github.com/Sevenstax/FreeV2G
"""

import asyncio
import threading
import time
import logging
from typing import Dict, Any, Optional

from everest.framework import Module, RuntimeSession, log
from Whitebeet import (
    WhiteBeetEthernet, WhiteBeetProtocol, MessageType,
    SlacRequestType, SlacResponseType, Iso15118RequestType,
    BoardSupportCommand, PowermeterCommand, RcdCommand, ConnectorLockCommand,
    PowerMeterData, BoardStatus
)

# Setup logging
logger = logging.getLogger(__name__)

class PyWhiteBeetEthDriver:
    """
    Python implementation of WhiteBeet Ethernet driver for EVerest
    
    This class implements all the required EVerest interfaces:
    - evse_board_support: Hardware control and monitoring
    - slac: SLAC powerline communication
    - charger: ISO 15118 V2G communication
    - powermeter: Energy measurement
    - rcd: RCD monitoring
    - connector_lock: Connector locking mechanism
    """
    
    def __init__(self):
        self._session = RuntimeSession()
        self._module = Module(self._session)
        
        # Update process name for logging
        log.update_process_name(self._module.info.id)
        
        # Get module configuration
        self._setup = self._module.say_hello()
        self._config = self._setup.configs.module
        
        # Initialize WhiteBeet communication
        self._whitebeet_eth = WhiteBeetEthernet(
            ip_address=self._config['ip_address'],
            port=self._config['port'],
            timeout=self._config['connection_timeout'] / 1000.0
        )
        self._whitebeet_protocol = WhiteBeetProtocol(self._whitebeet_eth)
        
        # Module state
        self._connected = False
        self._ready_event = threading.Event()
        self._running = False
        
        # Telemetry data
        self._last_powermeter_data: Optional[PowerMeterData] = None
        self._last_board_status: Optional[BoardStatus] = None
        
        # Setup interface implementations
        self._setup_implementations()
        
        # Initialize module
        self._module.init_done(self._ready)
        
        logger.info("PyWhiteBeetEthDriver initialized")
    
    def _setup_implementations(self):
        """Setup all interface command implementations"""
        
        # EVSE Board Support interface
        for cmd in self._module.implementations['evse_board_support'].commands:
            self._module.implement_command(
                'evse_board_support', cmd, getattr(self, f'_evse_board_support_{cmd}')
            )
        
        # SLAC interface
        for cmd in self._module.implementations['slac'].commands:
            self._module.implement_command(
                'slac', cmd, getattr(self, f'_slac_{cmd}')
            )
        
        # ISO 15118 Charger interface
        for cmd in self._module.implementations['charger'].commands:
            self._module.implement_command(
                'charger', cmd, getattr(self, f'_charger_{cmd}')
            )
        
        # Powermeter interface
        for cmd in self._module.implementations['powermeter'].commands:
            self._module.implement_command(
                'powermeter', cmd, getattr(self, f'_powermeter_{cmd}')
            )
        
        # RCD interface
        for cmd in self._module.implementations['rcd'].commands:
            self._module.implement_command(
                'rcd', cmd, getattr(self, f'_rcd_{cmd}')
            )
        
        # Connector Lock interface
        for cmd in self._module.implementations['connector_lock'].commands:
            self._module.implement_command(
                'connector_lock', cmd, getattr(self, f'_connector_lock_{cmd}')
            )
    
    def _ready(self):
        """Called when module is ready"""
        logger.info("PyWhiteBeetEthDriver ready")
        self._ready_event.set()
    
    def start(self):
        """Start the WhiteBeet driver"""
        self._ready_event.wait()
        self._running = True
        
        # Connect to WhiteBeet module
        if not self._connect_to_whitebeet():
            logger.error("Failed to connect to WhiteBeet module")
            return
        
        # Start telemetry if enabled
        if self._config.get('telemetry_enabled', True):
            self._start_telemetry()
        
        # Start SLAC if auto-start is enabled
        if self._config.get('enable_autostart', True):
            self._start_slac_auto()
        
        # Keep the module running
        try:
            while self._running:
                time.sleep(1.0)
        except KeyboardInterrupt:
            logger.info("Received interrupt signal")
        finally:
            self._shutdown()
    
    def _connect_to_whitebeet(self) -> bool:
        """Connect to WhiteBeet module"""
        max_retries = 3
        for attempt in range(max_retries):
            logger.info(f"Connecting to WhiteBeet (attempt {attempt + 1}/{max_retries})")
            
            if self._whitebeet_eth.connect():
                self._whitebeet_eth.start_receive_thread()
                self._connected = True
                logger.info("Successfully connected to WhiteBeet module")
                
                # Publish connection status
                self._module.publish_variable('evse_board_support', 'hardware_capabilities_response', {
                    'max_current_A_import': self._config['max_current'],
                    'min_current_A_import': 6.0,
                    'max_phase_count_import': self._config['phases'],
                    'min_phase_count_import': 1,
                    'max_current_A_export': 0.0,
                    'min_current_A_export': 0.0,
                    'max_phase_count_export': 0,
                    'min_phase_count_export': 0,
                    'supports_changing_phases_during_charging': False
                })
                
                return True
            
            time.sleep(2.0 * (attempt + 1))
        
        logger.error("Failed to connect to WhiteBeet module after all retries")
        return False
    
    def _start_telemetry(self):
        """Start telemetry data collection"""
        def telemetry_loop():
            interval = self._config.get('telemetry_interval', 1000) / 1000.0
            
            while self._running and self._connected:
                try:
                    # Get powermeter data
                    power_data = self._whitebeet_protocol.get_powermeter_data()
                    if power_data:
                        self._last_powermeter_data = power_data
                        self._publish_powermeter_data(power_data)
                    
                    time.sleep(interval)
                except Exception as e:
                    logger.error(f"Error in telemetry loop: {e}")
                    time.sleep(interval)
        
        telemetry_thread = threading.Thread(target=telemetry_loop, daemon=True)
        telemetry_thread.start()
        logger.info("Telemetry started")
    
    def _start_slac_auto(self):
        """Auto-start SLAC if configured"""
        def slac_auto_start():
            time.sleep(2.0)  # Give connection time to stabilize
            if self._running and self._connected:
                logger.info("Auto-starting SLAC matching")
                self._slac_start_matching({})
        
        threading.Thread(target=slac_auto_start, daemon=True).start()
    
    def _shutdown(self):
        """Shutdown the driver"""
        self._running = False
        if self._connected:
            self._whitebeet_eth.disconnect()
            self._connected = False
        logger.info("PyWhiteBeetEthDriver shutdown complete")
    
    def _publish_powermeter_data(self, data: PowerMeterData):
        """Publish powermeter data to EVerest"""
        powermeter_data = {
            'timestamp': time.time(),
            'meter_id': self._config['evse_id'],
            'phase_seq_error': False,
            'energy_Wh_import': {
                'total': data.energy_active_import,
                'L1': data.energy_active_import / 3,
                'L2': data.energy_active_import / 3,
                'L3': data.energy_active_import / 3
            },
            'energy_Wh_export': {
                'total': data.energy_active_export,
                'L1': data.energy_active_export / 3,
                'L2': data.energy_active_export / 3,
                'L3': data.energy_active_export / 3
            },
            'power_W': {
                'total': data.power_active,
                'L1': data.power_active / 3,
                'L2': data.power_active / 3,
                'L3': data.power_active / 3
            },
            'voltage_V': {
                'L1': data.voltage_l1,
                'L2': data.voltage_l2,
                'L3': data.voltage_l3
            },
            'VAR': {
                'total': data.power_reactive,
                'L1': data.power_reactive / 3,
                'L2': data.power_reactive / 3,
                'L3': data.power_reactive / 3
            },
            'current_A': {
                'L1': data.current_l1,
                'L2': data.current_l2,
                'L3': data.current_l3,
                'N': 0.0
            },
            'frequency_Hz': {
                'L1': data.frequency,
                'L2': data.frequency,
                'L3': data.frequency
            }
        }
        
        self._module.publish_variable('powermeter', 'powermeter', powermeter_data)
    
    # ============================================================================
    # EVSE Board Support Interface Implementation
    # ============================================================================
    
    def _evse_board_support_setup(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Setup the EVSE board support"""
        logger.info("EVSE board support setup")
        
        # Return hardware capabilities
        return {
            'max_current_A_import': self._config['max_current'],
            'min_current_A_import': 6.0,
            'max_phase_count_import': self._config['phases'],
            'min_phase_count_import': 1,
            'max_current_A_export': 0.0,
            'min_current_A_export': 0.0,
            'max_phase_count_export': 0,
            'min_phase_count_export': 0,
            'supports_changing_phases_during_charging': False
        }
    
    def _evse_board_support_enable(self, args: Dict[str, Any]) -> bool:
        """Enable the EVSE"""
        logger.info("Enabling EVSE")
        
        if not self._connected:
            logger.error("Not connected to WhiteBeet")
            return False
        
        success = self._whitebeet_protocol.enable_relay()
        if success:
            self._module.publish_variable('evse_board_support', 'event', {
                'event': 'enabled'
            })
        
        return success
    
    def _evse_board_support_disable(self, args: Dict[str, Any]) -> bool:
        """Disable the EVSE"""
        logger.info("Disabling EVSE")
        
        if not self._connected:
            logger.error("Not connected to WhiteBeet")
            return False
        
        success = self._whitebeet_protocol.disable_relay()
        if success:
            self._module.publish_variable('evse_board_support', 'event', {
                'event': 'disabled'
            })
        
        return success
    
    def _evse_board_support_allow_power_on(self, args: Dict[str, Any]) -> bool:
        """Allow power on"""
        logger.info("Allow power on")
        return self._evse_board_support_enable(args)
    
    def _evse_board_support_force_unlock(self, args: Dict[str, Any]) -> bool:
        """Force unlock connector"""
        logger.info("Force unlock connector")
        return self._connector_lock_unlock(args)
    
    # ============================================================================
    # SLAC Interface Implementation
    # ============================================================================
    
    def _slac_reset(self, args: Dict[str, Any]) -> bool:
        """Reset SLAC"""
        logger.info("SLAC reset")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_slac_request(SlacRequestType.RESET)
    
    def _slac_start_matching(self, args: Dict[str, Any]) -> bool:
        """Start SLAC matching"""
        logger.info("Starting SLAC matching")
        
        if not self._connected:
            return False
        
        success = self._whitebeet_protocol.start_slac_matching()
        if success:
            self._module.publish_variable('slac', 'state', 'MATCHING')
        
        return success
    
    def _slac_stop_matching(self, args: Dict[str, Any]) -> bool:
        """Stop SLAC matching"""
        logger.info("Stopping SLAC matching")
        
        if not self._connected:
            return False
        
        success = self._whitebeet_protocol.stop_slac_matching()
        if success:
            self._module.publish_variable('slac', 'state', 'UNMATCHED')
        
        return success
    
    # ============================================================================
    # ISO 15118 Charger Interface Implementation
    # ============================================================================
    
    def _charger_setup(self, args: Dict[str, Any]) -> bool:
        """Setup ISO 15118 charger"""
        logger.info("ISO 15118 charger setup")
        return True
    
    def _charger_session_setup(self, args: Dict[str, Any]) -> bool:
        """Setup charging session"""
        logger.info("Setting up charging session")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_iso15118_request(Iso15118RequestType.START_SESSION)
    
    def _charger_certificate_response(self, args: Dict[str, Any]) -> bool:
        """Handle certificate response"""
        logger.info("Certificate response")
        return True
    
    def _charger_authorization_response(self, args: Dict[str, Any]) -> bool:
        """Handle authorization response"""
        logger.info("Authorization response")
        return True
    
    def _charger_stop_charging(self, args: Dict[str, Any]) -> bool:
        """Stop charging"""
        logger.info("Stopping charging")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_iso15118_request(Iso15118RequestType.STOP_SESSION)
    
    def _charger_pause_charging(self, args: Dict[str, Any]) -> bool:
        """Pause charging"""
        logger.info("Pausing charging")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_iso15118_request(Iso15118RequestType.PAUSE_SESSION)
    
    # ============================================================================
    # Powermeter Interface Implementation
    # ============================================================================
    
    def _powermeter_start_transaction(self, args: Dict[str, Any]) -> bool:
        """Start power measurement transaction"""
        logger.info("Starting powermeter transaction")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_powermeter_request(PowermeterCommand.START_MEASUREMENT)
    
    def _powermeter_stop_transaction(self, args: Dict[str, Any]) -> bool:
        """Stop power measurement transaction"""
        logger.info("Stopping powermeter transaction")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_powermeter_request(PowermeterCommand.STOP_MEASUREMENT)
    
    # ============================================================================
    # RCD Interface Implementation
    # ============================================================================
    
    def _rcd_enable(self, args: Dict[str, Any]) -> bool:
        """Enable RCD monitoring"""
        logger.info("Enabling RCD")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_rcd_command(RcdCommand.ENABLE, True)
    
    def _rcd_disable(self, args: Dict[str, Any]) -> bool:
        """Disable RCD monitoring"""
        logger.info("Disabling RCD")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_rcd_command(RcdCommand.DISABLE, False)
    
    def _rcd_self_test(self, args: Dict[str, Any]) -> bool:
        """Perform RCD self test"""
        logger.info("RCD self test")
        
        if not self._connected:
            return False
        
        return self._whitebeet_eth.send_rcd_command(RcdCommand.TEST)
    
    # ============================================================================
    # Connector Lock Interface Implementation
    # ============================================================================
    
    def _connector_lock_lock(self, args: Dict[str, Any]) -> bool:
        """Lock the connector"""
        logger.info("Locking connector")
        
        if not self._connected:
            return False
        
        success = self._whitebeet_protocol.lock_connector()
        if success:
            self._module.publish_variable('connector_lock', 'state', 'Locked')
        
        return success
    
    def _connector_lock_unlock(self, args: Dict[str, Any]) -> bool:
        """Unlock the connector"""
        logger.info("Unlocking connector")
        
        if not self._connected:
            return False
        
        success = self._whitebeet_protocol.unlock_connector()
        if success:
            self._module.publish_variable('connector_lock', 'state', 'Unlocked')
        
        return success


def main():
    """Main entry point"""
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # Create and start the driver
    driver = PyWhiteBeetEthDriver()
    try:
        driver.start()
    except KeyboardInterrupt:
        logger.info("Received keyboard interrupt")
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
    finally:
        driver._shutdown()


if __name__ == "__main__":
    main()
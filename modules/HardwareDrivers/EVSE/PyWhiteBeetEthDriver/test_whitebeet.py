#!/usr/bin/env python3
"""
WhiteBeet Test Script

This script tests the WhiteBeet communication module independently
of the EVerest framework for development and debugging.
"""

import sys
import time
import logging
from pathlib import Path

# Add the module directory to Python path
sys.path.insert(0, str(Path(__file__).parent))

from Whitebeet import (
    WhiteBeetEthernet, WhiteBeetProtocol, MessageType,
    SlacRequestType, PowermeterCommand
)

# Setup logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)

def test_whitebeet_connection(ip_address: str = "192.168.1.100", port: int = 15118):
    """Test basic WhiteBeet connection and communication"""
    
    logger.info("=== WhiteBeet Connection Test ===")
    
    # Create communication objects
    ethernet = WhiteBeetEthernet(ip_address, port, timeout=5.0)
    protocol = WhiteBeetProtocol(ethernet)
    
    try:
        # Test connection
        logger.info(f"Connecting to WhiteBeet at {ip_address}:{port}")
        if not ethernet.connect():
            logger.error("Failed to connect to WhiteBeet")
            return False
        
        logger.info("Successfully connected!")
        
        # Start receive thread
        ethernet.start_receive_thread()
        
        # Test basic communication
        logger.info("Testing basic communication...")
        
        # Test 1: Get powermeter data
        logger.info("Test 1: Getting powermeter data")
        power_data = protocol.get_powermeter_data(timeout=3.0)
        if power_data:
            logger.info(f"Power data received: {power_data}")
        else:
            logger.warning("No powermeter data received")
        
        # Test 2: Start SLAC matching
        logger.info("Test 2: Starting SLAC matching")
        if protocol.start_slac_matching(timeout=3.0):
            logger.info("SLAC matching started successfully")
        else:
            logger.warning("Failed to start SLAC matching")
        
        # Test 3: Stop SLAC matching
        logger.info("Test 3: Stopping SLAC matching")
        if protocol.stop_slac_matching(timeout=3.0):
            logger.info("SLAC matching stopped successfully")
        else:
            logger.warning("Failed to stop SLAC matching")
        
        # Keep connection alive for a bit
        logger.info("Keeping connection alive for 5 seconds...")
        time.sleep(5.0)
        
        logger.info("=== Test completed successfully ===")
        return True
        
    except Exception as e:
        logger.error(f"Test failed with exception: {e}")
        return False
        
    finally:
        # Cleanup
        ethernet.disconnect()
        logger.info("Disconnected from WhiteBeet")

def test_message_parsing():
    """Test message parsing functionality"""
    
    logger.info("=== Message Parsing Test ===")
    
    # Test message creation and parsing
    ethernet = WhiteBeetEthernet("test", 1234)
    
    # Test SLAC request
    result = ethernet.send_slac_request(SlacRequestType.START_MATCHING, b'\x01\x02\x03')
    logger.info(f"SLAC request creation: {'OK' if not result else 'Failed (expected - no connection)'}")
    
    # Test powermeter request
    result = ethernet.send_powermeter_request(PowermeterCommand.GET_MEASUREMENT)
    logger.info(f"Powermeter request creation: {'OK' if not result else 'Failed (expected - no connection)'}")
    
    logger.info("=== Message parsing test completed ===")

def main():
    """Main test function"""
    
    print("WhiteBeet Python Driver Test")
    print("============================")
    
    if len(sys.argv) > 1:
        ip_address = sys.argv[1]
    else:
        ip_address = input("Enter WhiteBeet IP address (default: 192.168.1.100): ").strip()
        if not ip_address:
            ip_address = "192.168.1.100"
    
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    else:
        port_str = input("Enter WhiteBeet port (default: 15118): ").strip()
        port = int(port_str) if port_str else 15118
    
    print(f"\nTesting connection to {ip_address}:{port}")
    print("Press Ctrl+C to interrupt\n")
    
    try:
        # Test message parsing (offline)
        test_message_parsing()
        
        print()
        
        # Test actual connection
        test_whitebeet_connection(ip_address, port)
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"\nTest failed: {e}")
        logger.exception("Detailed error information:")

if __name__ == "__main__":
    main()
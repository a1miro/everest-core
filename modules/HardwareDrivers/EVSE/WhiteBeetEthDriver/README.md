# WhiteBeetEthDriver

A hardware driver module for EVerest that connects the ISO 15118/SLAC stack to the 8devices WHITE-beet-EI ISO15118 EVSE module via Ethernet communication.

## Overview

The WhiteBeetEthDriver module provides a complete interface between EVerest and the WHITE-beet hardware platform, enabling:

- **ISO 15118 V2G Communication**: High-level vehicle-to-grid communication protocol
- **SLAC (Signal Level Attenuation Characterization)**: Powerline communication for EV-EVSE association
- **EVSE Board Support**: Hardware control and monitoring functions
- **Powermeter Integration**: Energy measurement and monitoring
- **AC RCD Protection**: Residual current device monitoring
- **Connector Lock Control**: Physical connector locking mechanism

## Architecture

The module is built using a layered architecture:

### Communication Layer
- **WhiteBeetEthernet**: TCP/IP connection management with the WHITE-beet module
- **WhiteBeetProtocol**: Message protocol implementation for WHITE-beet HCI
- **EthernetFraming**: Low-level Ethernet frame handling and TCP packet construction

### Interface Implementations
- **ISO15118_charger**: V2G communication interface implementation
- **slac**: SLAC protocol interface for powerline communication
- **evse_board_support**: Hardware control and status interface
- **powermeter**: Energy measurement interface
- **ac_rcd**: RCD monitoring interface
- **connector_lock**: Connector locking mechanism interface

## Configuration

The module supports the following configuration parameters:

```yaml
# Network configuration
ip_address: "192.168.1.100"  # WHITE-beet IP address
port: 15118                  # WHITE-beet TCP port
connection_timeout: 5000     # Connection timeout in milliseconds

# Hardware configuration
evse_id: "DE*ABC*E12345"    # EVSE identification
max_current: 32             # Maximum current in Amperes
max_voltage: 400            # Maximum voltage in Volts
phases: 3                   # Number of phases (1 or 3)

# SLAC configuration
enable_autostart: true      # Auto-start SLAC on module init
matching_timeout: 10        # SLAC matching timeout in seconds

# RCD configuration
rcd_enabled: true          # Enable RCD monitoring
rcd_threshold: 30          # RCD trip threshold in mA

# Connector lock configuration
lock_timeout: 5000         # Lock operation timeout in milliseconds
```

## Usage

To use the WhiteBeetEthDriver in your EVerest configuration:

```yaml
main:
  module: WhiteBeetEthDriver
  config_implementation:
    main:
      ip_address: "192.168.1.100"
      port: 15118
      evse_id: "DE*ABC*E12345"
      max_current: 32
```

## Hardware Requirements

- 8devices WHITE-beet-EI ISO15118 EVSE module
- Ethernet connection between the host and WHITE-beet module
- Compatible powerline communication hardware for SLAC

## Protocol Details

The module implements the WHITE-beet HCI (Host Control Interface) protocol, which consists of:

1. **Message Framing**: 8-byte header with length, type, sequence number, and reserved fields
2. **Message Types**: Categorized by function (SLAC, ISO15118, Board Support, etc.)
3. **Request-Response Pattern**: Synchronous communication for most operations
4. **Asynchronous Notifications**: Status updates and error indications

## Development Reference

This implementation is based on the FreeV2G project (https://github.com/Sevenstax/FreeV2G), specifically the Python host application that demonstrates WHITE-beet integration.

## Dependencies

- EVerest framework
- Standard C++ threading libraries
- POSIX socket APIs (Linux/Unix)

## Build Requirements

The module requires:
- C++17 compatible compiler
- CMake 3.14 or later
- EVerest development environment
- POSIX-compliant operating system

## License

This module follows the EVerest project licensing terms.
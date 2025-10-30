# PyWhiteBeetEthDriver - Python EVerest Module

A Python implementation of the WhiteBeet Ethernet driver for EVerest, providing connectivity to the 8devices WHITE-beet-EI ISO15118 EVSE module via Ethernet.

## Overview

This module implements all the required EVerest interfaces to integrate WhiteBeet hardware with the EVerest charging framework:

- **evse_board_support**: Hardware control and monitoring
- **slac**: SLAC powerline communication  
- **charger**: ISO 15118 V2G communication
- **powermeter**: Energy measurement
- **rcd**: RCD monitoring
- **connector_lock**: Connector locking mechanism

## Architecture

### Core Components

- **Whitebeet.py**: Core communication library implementing the WhiteBeet HCI protocol
- **module.py**: Main EVerest module implementation with interface handlers
- **manifest.yaml**: EVerest module configuration and interface definitions

### Key Classes

- **WhiteBeetEthernet**: Low-level TCP/IP communication with WhiteBeet module
- **WhiteBeetProtocol**: High-level protocol implementation with response handling
- **PyWhiteBeetEthDriver**: Main EVerest module class implementing all interfaces

## Features

### WhiteBeet Communication
- **TCP/IP Protocol**: Robust network communication with connection management
- **HCI Protocol**: Complete implementation of WhiteBeet Host Control Interface
- **Message Threading**: Asynchronous message handling with callback system
- **Error Handling**: Comprehensive error handling and recovery mechanisms

### EVerest Integration
- **Interface Compliance**: Full implementation of all 6 required EVerest interfaces
- **Event Publishing**: Real-time status updates and telemetry data
- **Configuration Management**: Flexible configuration via EVerest manifest
- **Logging Integration**: Integrated with EVerest logging system

### Hardware Support
- **Relay Control**: Enable/disable main power relay
- **SLAC Management**: Start/stop powerline communication matching
- **ISO 15118**: V2G communication session management
- **Power Measurement**: Real-time energy monitoring and reporting
- **RCD Monitoring**: Residual current device safety monitoring
- **Connector Lock**: Physical connector locking control

## Configuration

Configure the module via the EVerest manifest or configuration files:

```yaml
ip_address: "192.168.1.100"    # WhiteBeet module IP
port: 15118                    # WhiteBeet TCP port
connection_timeout: 5000       # Connection timeout (ms)
evse_id: "DE*PYW*E00001"      # EVSE identifier
max_current: 32                # Maximum current (A)
max_voltage: 400               # Maximum voltage (V)
phases: 3                      # Number of phases
enable_autostart: true         # Auto-start SLAC
telemetry_enabled: true        # Enable telemetry
```

## Installation & Usage

### In EVerest Framework

1. **Copy Module**: Place in `modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/`
2. **Configure**: Update your EVerest configuration to use PyWhiteBeetEthDriver
3. **Build**: Run EVerest build process
4. **Deploy**: Start EVerest with WhiteBeet configuration

### Standalone Testing

Test the communication independently:

```bash
cd modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/
python3 test_whitebeet.py [IP_ADDRESS] [PORT]
```

Example:
```bash
python3 test_whitebeet.py 192.168.1.100 15118
```

## Development

### File Structure
```
PyWhiteBeetEthDriver/
├── manifest.yaml           # EVerest module configuration
├── module.py               # Main EVerest module implementation
├── Whitebeet.py            # Core WhiteBeet communication library
├── test_whitebeet.py       # Standalone test script
├── requirements.txt        # Python dependencies
└── README.md              # This file
```

### Adding New Features

1. **Extend Protocol**: Add new message types to `Whitebeet.py`
2. **Update Interfaces**: Implement new commands in `module.py`
3. **Test Changes**: Use `test_whitebeet.py` for validation
4. **Update Manifest**: Add new configuration parameters if needed

### Protocol Details

The module implements the WhiteBeet HCI protocol:

- **Message Format**: `[length:2][type:1][seq:1][reserved:4][payload:...]`
- **Message Types**: SLAC, ISO15118, Board Support, Powermeter, RCD, Connector Lock
- **Response Handling**: Synchronous and asynchronous message processing
- **Error Handling**: Comprehensive error detection and recovery

## Network Setup

Ensure proper network configuration:

```bash
# Test connectivity
ping 192.168.1.100

# Test port availability  
telnet 192.168.1.100 15118

# Check firewall settings
# Ensure TCP port 15118 is open
```

## Troubleshooting

### Common Issues

**Connection Failed**:
- Verify WhiteBeet IP address and network connectivity
- Check firewall settings for port 15118
- Ensure WhiteBeet module is powered and configured

**SLAC Timeout**:
- Check powerline communication setup
- Verify EV compatibility
- Increase `matching_timeout` parameter

**No Telemetry Data**:
- Enable telemetry in configuration
- Check WhiteBeet powermeter functionality
- Verify network stability

### Debug Mode

Enable debug logging for detailed information:

```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

### Log Analysis

Key log messages to monitor:
- `Connected to WhiteBeet at...` - Connection established
- `SLAC matching started` - Powerline communication active
- `Power data received` - Telemetry working
- `Error in receive loop` - Communication issues

## Dependencies

- **Python 3.7+**: Required for dataclasses and typing support
- **EVerest Framework**: For module integration (everest.framework)
- **Standard Library**: socket, threading, logging, json, struct

No external Python packages required beyond EVerest framework.

## Based on FreeV2G

This implementation is based on the FreeV2G project:
- **Repository**: https://github.com/Sevenstax/FreeV2G
- **Reference**: Python host application for WhiteBeet control
- **Protocol**: WhiteBeet HCI implementation patterns

## License

Apache 2.0 License - See EVerest project licensing terms.

## Support

For issues and questions:
- **WhiteBeet Hardware**: Consult 8devices documentation
- **EVerest Integration**: Check EVerest community resources
- **Protocol Implementation**: Review FreeV2G reference code
- **This Module**: Check logs and use debug mode for troubleshooting
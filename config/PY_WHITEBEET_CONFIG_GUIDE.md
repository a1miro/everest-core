# PyWhiteBeetEthDriver Configuration Guide

This guide explains how to configure and use the Python WhiteBeet Ethernet Driver configurations for EVerest.

## Available Configurations

### 1. `config-py-whitebeet.yaml` - Production Configuration
**Purpose**: Full-featured production configuration with comprehensive settings
**Use Case**: Production deployments with actual WhiteBeet hardware
**Features**:
- Complete safety features enabled
- Full telemetry and monitoring
- 3-phase AC charging support
- Comprehensive error handling
- Production-grade timeouts and limits

### 2. `config-py-whitebeet-test.yaml` - Testing Configuration  
**Purpose**: Simplified configuration for hardware testing and validation
**Use Case**: Integration testing with real WhiteBeet hardware
**Features**:
- Single-phase operation for simpler testing
- Reduced current limits (16A) for safety
- Shorter timeouts for faster testing
- Debug logging enabled
- Fault tolerance for testing scenarios

### 3. `config-py-whitebeet-dev.yaml` - Development Configuration
**Purpose**: Development and debugging configuration
**Use Case**: Software development, debugging, and offline development
**Features**:
- Can use localhost (127.0.0.1) for mock hardware
- Extensive debug logging
- Mock mode support
- Development-friendly timeouts
- Detailed error reporting

## Configuration Parameters

### Network Settings
```yaml
ip_address: "192.168.1.100"    # WhiteBeet module IP address
port: 15118                    # WhiteBeet HCI TCP port (default 15118)
connection_timeout: 5000       # Connection timeout in milliseconds
reconnect_interval: 10000      # Reconnection attempt interval
```

### EVSE Identity
```yaml
evse_id: "DE*WB*E12345"       # Must match evse_manager configuration
vendor_id: "8devices"         # Hardware vendor
model_name: "WHITE-beet-EI"   # Hardware model
firmware_version: "1.0.0"    # Expected firmware version
```

### Electrical Configuration
```yaml
max_current_l1: 32.0          # Phase L1 max current (Amperes)
max_current_l2: 32.0          # Phase L2 max current (Amperes)
max_current_l3: 32.0          # Phase L3 max current (Amperes)
max_voltage: 400.0            # Maximum voltage (Volts)
nominal_voltage: 230.0        # Nominal AC voltage per phase
frequency: 50.0               # Grid frequency (Hz)
phases: 3                     # Number of phases (1 or 3)
```

### SLAC Configuration
```yaml
slac_enabled: true            # Enable SLAC functionality
slac_matching_timeout_s: 10   # SLAC matching timeout (seconds)
enable_autostart: true       # Auto-start SLAC on initialization
```

### Safety Features
```yaml
rcd_enabled: true             # Enable RCD protection
rcd_threshold_ma: 30          # RCD trip threshold (milliamperes)
connector_lock_enabled: true  # Enable connector lock
lock_timeout_ms: 5000        # Lock operation timeout
```

### ISO 15118 Settings
```yaml
iso15118_enabled: true                    # Enable ISO 15118 HLC
iso15118_ac_three_phase_supported: true  # 3-phase AC support
iso15118_ac_single_phase_supported: true # 1-phase AC support
iso15118_dc_supported: false             # DC charging (not supported)
iso15118_free_service: true              # Free charging service
```

### Debug and Development
```yaml
debug_mode: false             # Enable detailed debug logging
log_raw_messages: false       # Log raw HCI messages
mock_mode: false             # Use mock hardware responses (dev only)
telemetry_enabled: true      # Enable telemetry collection
```

## Usage Instructions

### 1. Production Deployment

```bash
# Use the production configuration
cd /path/to/everest-core
./build/run-scripts/run-manager.py --config config/config-py-whitebeet.yaml
```

**Prerequisites**:
- WhiteBeet hardware connected to network
- Correct IP address configured in config
- Network connectivity verified
- Safety systems properly installed

### 2. Hardware Testing

```bash
# Use the testing configuration
./build/run-scripts/run-manager.py --config config/config-py-whitebeet-test.yaml
```

**Testing Steps**:
1. Run standalone test first: `python3 modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/test_whitebeet.py`
2. Verify hardware connectivity
3. Check logs for any issues
4. Run EVerest with test config
5. Monitor system behavior

### 3. Development and Debugging

```bash
# Use the development configuration
./build/run-scripts/run-manager.py --config config/config-py-whitebeet-dev.yaml
```

**Development Workflow**:
1. Start with `mock_mode: true` for initial development
2. Use standalone testing to verify communication
3. Enable `debug_mode: true` for detailed logging
4. Switch to real hardware when available
5. Monitor logs and telemetry for optimization

## Network Configuration

### Hardware Network Setup
```bash
# Example network configuration for WhiteBeet
# Set WhiteBeet IP: 192.168.1.100
# Set EVerest host IP: 192.168.1.10
# Ensure both are on same subnet

# Test connectivity
ping 192.168.1.100
telnet 192.168.1.100 15118
```

### Firewall Configuration
```bash
# Allow EVerest to connect to WhiteBeet
sudo ufw allow out 15118
sudo ufw allow from 192.168.1.100
```

## Troubleshooting

### Common Issues and Solutions

#### Connection Failed
```yaml
# Check network settings
ip_address: "192.168.1.100"  # Verify IP is correct
port: 15118                  # Verify port is correct
connection_timeout: 5000     # Increase if network is slow
```

#### SLAC Not Working
```yaml
# Adjust SLAC settings
slac_matching_timeout_s: 15  # Increase timeout
enable_autostart: false      # Manual start for debugging
```

#### Power Measurement Issues
```yaml
# Configure power meter
powermeter_enabled: true
powermeter_poll_interval_ms: 2000  # Slower polling
power_smoothing_enabled: true      # Enable smoothing
```

#### Lock Mechanism Problems
```yaml
# Adjust lock settings
lock_timeout_ms: 10000      # Increase timeout
unlock_on_error: true       # Auto-unlock on errors
```

### Debug Logging

Enable detailed logging for troubleshooting:

```yaml
logging:
  loggers:
    - logger: "py_whitebeet_driver"
      level: "debug"          # Maximum detail
    - logger: "manager"  
      level: "debug"
```

Check logs:
```bash
tail -f logs/py-whitebeet-*.log
```

### Mock Mode for Development

For development without hardware:

```yaml
py_whitebeet_dev:
  config_implementation:
    main:
      ip_address: "127.0.0.1"  # Localhost
      mock_mode: true          # Enable mock responses
      debug_mode: true         # Enable debug logging
```

## Integration with EVerest

### Module Dependencies
The PyWhiteBeetEthDriver requires these EVerest modules:
- `EvseManager` - Core EVSE management
- `EnergyManager` - Energy flow management  
- `Auth` - Authentication handling

### Interface Connections
The driver provides these interfaces:
- `evse_board_support` - Hardware control
- `slac` - Powerline communication
- `charger` - ISO 15118 communication
- `powermeter` - Power measurement
- `rcd` - Residual current detection
- `connector_lock` - Physical connector lock

### Configuration Validation

Before deployment, validate configuration:

```python
# Run configuration validator
python3 modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/validate_config.py config/config-py-whitebeet.yaml
```

## Best Practices

### Security
- Use private network segments for EVSE communication
- Change default passwords on WhiteBeet hardware
- Enable firewalls and access controls
- Monitor network traffic for anomalies

### Reliability
- Configure appropriate timeouts for your network
- Enable reconnection logic
- Use conservative current limits initially
- Monitor system logs regularly

### Performance
- Tune polling intervals based on requirements
- Enable telemetry for monitoring
- Use power smoothing for stable measurements
- Optimize network latency

### Maintenance
- Keep logs for troubleshooting
- Monitor hardware health
- Update firmware regularly
- Test configurations before deployment

## Configuration Examples

### Single-Phase 16A Installation
```yaml
phases: 1
max_current_l1: 16.0
max_current_l2: 0.0
max_current_l3: 0.0
iso15118_ac_single_phase_supported: true
iso15118_ac_three_phase_supported: false
```

### Three-Phase 32A Installation
```yaml
phases: 3
max_current_l1: 32.0
max_current_l2: 32.0  
max_current_l3: 32.0
iso15118_ac_single_phase_supported: true
iso15118_ac_three_phase_supported: true
```

### High-Security Installation
```yaml
rcd_enabled: true
rcd_threshold_ma: 20          # Stricter than standard 30mA
emergency_stop_enabled: true
overvoltage_protection: true
undervoltage_protection: true
overcurrent_protection: true
overtemperature_protection: true
```

## Support and Documentation

- **Module Documentation**: `modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/README.md`
- **Python Development Guide**: `PYTHON_MODULE_GUIDE.md`
- **EVerest Documentation**: https://everest.github.io/
- **WhiteBeet Hardware Manual**: Refer to 8devices documentation

For issues and questions, check the module logs and refer to the troubleshooting section above.
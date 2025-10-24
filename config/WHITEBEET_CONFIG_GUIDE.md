# WhiteBeet EVerest Configuration Guide

This directory contains three EVerest configuration files for the WhiteBeetEthDriver module:

## Configuration Files

### 1. `config-whitebeet-dev.yaml` - Development & Testing
**Use this for**: Initial development, testing connectivity, debugging

**Features**:
- Minimal module set for faster startup
- Conservative electrical settings (16A, single phase)
- RCD and SLAC disabled initially
- Maximum debug logging
- Always-accept authentication for testing
- Enhanced error reporting

**Setup**:
1. Update `ip_address` to match your WhiteBeet module IP
2. Ensure WhiteBeet module is accessible on network
3. Run: `./build/bin/manager --conf config/config-whitebeet-dev.yaml`

### 2. `config-whitebeet-simple.yaml` - Basic Production
**Use this for**: Simple AC charging station deployment

**Features**:
- Full EVSE functionality with WhiteBeet driver
- 3-phase, 32A AC charging
- RCD monitoring enabled
- SLAC auto-start enabled
- Energy management
- Basic authentication (Plug & Charge)
- Moderate logging

**Setup**:
1. Configure network settings for WhiteBeet module
2. Adjust `evse_id` to match your station ID
3. Configure authentication as needed
4. Run: `./build/bin/manager --conf config/config-whitebeet-simple.yaml`

### 3. `config-whitebeet-iso15118.yaml` - Full Featured
**Use this for**: Complete charging station with all features

**Features**:
- Full ISO 15118 High Level Communication
- OCPP backend connectivity
- Certificate management (V2G security)
- Advanced energy management
- Complete error handling
- System management features
- Production-ready logging

**Setup**:
1. Configure all network and hardware parameters
2. Set up OCPP backend connection
3. Configure security certificates
4. Customize for your deployment requirements
5. Run: `./build/bin/manager --conf config/config-whitebeet-iso15118.yaml`

## Configuration Parameters

### WhiteBeet Driver Parameters

| Parameter | Description | Default | Notes |
|-----------|-------------|---------|--------|
| `ip_address` | WhiteBeet module IP | `192.168.1.100` | **Must be configured** |
| `port` | WhiteBeet TCP port | `15118` | Usually don't change |
| `connection_timeout` | Connection timeout (ms) | `5000` | Increase for slow networks |
| `evse_id` | EVSE identifier | `DE*ABC*E12345` | Must match station ID |
| `max_current` | Maximum current (A) | `32` | Set per hardware limits |
| `max_voltage` | Maximum voltage (V) | `400` | Set per installation |
| `phases` | Number of phases | `3` | 1 or 3 |
| `enable_autostart` | Auto-start SLAC | `true` | Set false for manual control |
| `matching_timeout` | SLAC timeout (s) | `10` | SLAC association timeout |
| `rcd_enabled` | Enable RCD monitoring | `true` | Safety feature |
| `rcd_threshold` | RCD threshold (mA) | `30` | Per safety regulations |
| `lock_timeout` | Lock timeout (ms) | `5000` | Connector lock operation |
| `telemetry_enabled` | Enable telemetry | `true` | Performance monitoring |
| `telemetry_interval` | Telemetry interval (ms) | `1000` | Update frequency |

## Network Setup

### WhiteBeet Module Network Configuration
The WhiteBeet module must be configured with a static IP address accessible from the EVerest host:

```bash
# Example network setup (adjust to your network)
WhiteBeet IP: 192.168.1.100
EVerest Host: 192.168.1.50
Subnet: 192.168.1.0/24
Gateway: 192.168.1.1
```

### Firewall Considerations
Ensure TCP port 15118 is open between EVerest host and WhiteBeet module:
```bash
# Test connectivity
telnet 192.168.1.100 15118
```

## Testing Procedure

### Phase 1: Basic Connectivity
1. Use `config-whitebeet-dev.yaml`
2. Verify network connectivity to WhiteBeet
3. Check EVerest logs for connection establishment
4. Test basic module initialization

### Phase 2: Interface Testing  
1. Test each interface individually:
   - Board support (relay control)
   - Powermeter (energy measurement)
   - Connector lock (if available)
2. Verify telemetry data flow
3. Test error handling

### Phase 3: Protocol Testing
1. Enable SLAC (`enable_autostart: true`)
2. Test ISO 15118 basic communication
3. Test with actual EV (if available)
4. Verify charging session flow

### Phase 4: Production Deployment
1. Switch to `config-whitebeet-simple.yaml` or full config
2. Configure authentication system
3. Enable all safety features (RCD, etc.)
4. Perform acceptance testing

## Troubleshooting

### Common Issues

**Connection Failed**:
- Check WhiteBeet IP address and network connectivity
- Verify firewall settings
- Check WhiteBeet module status/configuration

**SLAC Timeout**:
- Increase `matching_timeout` parameter
- Check powerline communication setup
- Verify EV compatibility

**Authentication Issues**:
- Check token provider/validator configuration
- Verify EVSE ID consistency
- Review authentication flow logs

**Build Errors**:
- Ensure all dependencies are installed
- Check CMake configuration
- Verify EVerest framework version compatibility

### Log Analysis
Check these log files for debugging:
- `./logs/whitebeet-dev.log` - Main application log
- `./logs/whitebeet-dev-errors.log` - Error history
- Console output - Real-time status

Look for these key messages:
- `Successfully connected to WhiteBeet at...` - Connection OK
- `WhiteBeet receive thread started` - Communication active
- `SLAC matching completed` - PLC communication established
- `ISO15118 session started` - V2G communication active

## Support

For issues specific to:
- **WhiteBeet hardware**: Consult 8devices documentation
- **EVerest framework**: Check EVerest community resources  
- **ISO 15118 protocol**: Refer to ISO standard documentation
- **This driver implementation**: Review module source code and logs
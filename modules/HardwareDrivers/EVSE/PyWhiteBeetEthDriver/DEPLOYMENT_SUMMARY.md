# PyWhiteBeetEthDriver Installation Summary

## Quick Installation Methods

### Method 1: Automated Installation (Recommended)

```bash
# Clone the repository
git clone https://github.com/EVerest/everest-core.git
cd everest-core

# Run automated installation script
sudo ./modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/install_py_whitebeet.sh

# Configure for your environment
sudo nano /opt/everest/config/config-py-whitebeet.yaml

# Test installation
cd /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver
python3 test_whitebeet.py

# Start service
sudo systemctl start everest-py-whitebeet.service
sudo systemctl enable everest-py-whitebeet.service
```

### Method 2: Docker Deployment

```bash
# Build Docker image
docker build -t everest-py-whitebeet:latest .

# Run container
docker run -d \
    --name everest-py-whitebeet \
    --network host \
    -v $(pwd)/logs:/workspace/everest-core/logs \
    everest-py-whitebeet:latest
```

### Method 3: Manual Installation

```bash
# Install EVerest dependencies
sudo apt update
sudo apt install -y build-essential cmake python3 python3-pip libboost-all-dev

# Build EVerest
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# The PyWhiteBeetEthDriver module is automatically included
```

## File Structure After Installation

```
/opt/everest/
├── modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/
│   ├── module.py                    # Main EVerest module
│   ├── Whitebeet.py                 # Communication library
│   ├── manifest.yaml               # Module configuration
│   ├── test_whitebeet.py           # Standalone test
│   ├── requirements.txt            # Python dependencies
│   ├── README.md                   # Module documentation
│   ├── CMakeLists.txt              # Build configuration
│   └── install_py_whitebeet.sh     # Installation script
├── config/
│   ├── config-py-whitebeet.yaml         # Production config
│   ├── config-py-whitebeet-test.yaml    # Testing config
│   ├── config-py-whitebeet-dev.yaml     # Development config
│   └── PY_WHITEBEET_CONFIG_GUIDE.md     # Configuration guide
├── build/
│   └── run-scripts/
│       └── run-manager.py               # EVerest launcher
└── logs/
    └── py-whitebeet*.log                 # Log files
```

## Key Integration Points

### 1. Build System Integration
- **CMakeLists.txt**: Empty file (Python modules auto-discovered)
- **Parent CMakeLists.txt**: Added `ev_add_module(PyWhiteBeetEthDriver)`
- **Module Discovery**: EVerest automatically finds Python modules with `manifest.yaml`

### 2. EVerest Framework Integration
- **Module Registration**: Via `manifest.yaml` interface definitions
- **Interface Implementation**: All 6 interfaces implemented in `module.py`
- **Configuration**: YAML-based configuration with parameter validation
- **Logging**: Integrated with EVerest logging framework

### 3. System Integration
- **Systemd Service**: Automatic startup and monitoring
- **User Management**: Dedicated `everest` system user
- **File Permissions**: Proper security permissions
- **Firewall**: Port 15118 configured for WhiteBeet communication

## Installation Requirements

### Minimum System Requirements
- **OS**: Ubuntu 20.04+ / Debian 11+ / similar Linux
- **CPU**: ARM64 or x86_64
- **RAM**: 512MB minimum, 1GB+ recommended
- **Storage**: 2GB free space minimum
- **Network**: Ethernet interface for WhiteBeet communication

### Software Dependencies
- **Python 3.8+** with pip
- **EVerest Framework** (automatically installed)
- **CMake 3.20+** and build tools
- **Boost libraries** for EVerest
- **Standard Python libraries** (socket, threading, struct, json, logging)

### Network Configuration
- **WhiteBeet IP**: Typically 192.168.1.100
- **EVerest Host IP**: Configure static IP on same subnet
- **Port**: 15118 (WhiteBeet HCI protocol)
- **Firewall**: Allow port 15118 TCP

## Verification Steps

### 1. Build Verification
```bash
# Check module is included in build
grep -r "PyWhiteBeetEthDriver" /opt/everest/modules/

# Verify Python dependencies
python3 -c "import socket, threading, struct, time, json, logging"
```

### 2. Standalone Testing
```bash
# Test hardware communication
cd /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver
python3 test_whitebeet.py
```

### 3. EVerest Integration Testing
```bash
# Test with EVerest framework
cd /opt/everest
./build/run-scripts/run-manager.py --config config/config-py-whitebeet-test.yaml
```

### 4. Service Testing
```bash
# Test systemd service
sudo systemctl start everest-py-whitebeet.service
sudo systemctl status everest-py-whitebeet.service
sudo journalctl -u everest-py-whitebeet.service -f
```

## Deployment Configurations

### Development Environment
- **Config**: `config-py-whitebeet-dev.yaml`
- **Features**: Debug logging, mock mode support, localhost connection
- **Use Case**: Software development without hardware

### Testing Environment  
- **Config**: `config-py-whitebeet-test.yaml`
- **Features**: Single-phase, reduced current, debug enabled
- **Use Case**: Hardware integration testing

### Production Environment
- **Config**: `config-py-whitebeet.yaml` 
- **Features**: Full functionality, 3-phase support, comprehensive safety
- **Use Case**: Production charging stations

## Troubleshooting Quick Reference

### Connection Issues
```bash
# Test network connectivity
ping 192.168.1.100
telnet 192.168.1.100 15118

# Check firewall
sudo ufw status
```

### Module Issues
```bash
# Check module loading
tail -f /var/log/everest/py-whitebeet.log

# Test Python imports
python3 -c "import sys; sys.path.insert(0, '/opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver'); import module"
```

### Service Issues
```bash
# Check service status
sudo systemctl status everest-py-whitebeet.service
sudo journalctl -u everest-py-whitebeet.service --no-pager

# Restart service
sudo systemctl restart everest-py-whitebeet.service
```

## Support Resources

- **Installation Guide**: `INSTALLATION_GUIDE.md` - Comprehensive installation documentation
- **Configuration Guide**: `config/PY_WHITEBEET_CONFIG_GUIDE.md` - Configuration reference
- **Module Documentation**: `modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/README.md`
- **Python Development**: `PYTHON_MODULE_GUIDE.md` - Python module development guide
- **EVerest Documentation**: https://everest.github.io/

## Post-Installation Steps

1. **Configure Network**: Set static IP addresses for reliable communication
2. **Customize Configuration**: Edit YAML config files for your environment
3. **Test Hardware**: Verify communication with WhiteBeet device
4. **Monitor Logs**: Set up log monitoring and rotation
5. **Backup Configuration**: Create backups of working configurations
6. **Documentation**: Document your specific deployment details

The PyWhiteBeetEthDriver is now fully integrated into the EVerest ecosystem and ready for deployment!
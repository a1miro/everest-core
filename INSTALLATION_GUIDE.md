# PyWhiteBeetEthDriver Installation Guide

This guide explains how to install and deploy the PyWhiteBeetEthDriver on target systems.

## Table of Contents

1. [Installation Methods](#installation-methods)
2. [System Requirements](#system-requirements)
3. [EVerest Source Installation](#everest-source-installation)
4. [Docker Deployment](#docker-deployment)
5. [Package Installation](#package-installation)
6. [Configuration and Deployment](#configuration-and-deployment)
7. [Verification and Testing](#verification-and-testing)
8. [Troubleshooting](#troubleshooting)

## Installation Methods

The PyWhiteBeetEthDriver can be installed in several ways:

1. **Source Installation** - Build from EVerest source code (recommended for development)
2. **Docker Deployment** - Use containerized deployment (recommended for production)
3. **Package Installation** - Install from pre-built packages (when available)
4. **Manual Installation** - Copy files directly to target system

## System Requirements

### Hardware Requirements
- **CPU**: ARM64 or x86_64 processor
- **RAM**: Minimum 512MB, recommended 1GB+
- **Storage**: Minimum 2GB free space
- **Network**: Ethernet interface for WhiteBeet communication

### Software Requirements
- **OS**: Linux (Ubuntu 20.04+, Debian 11+, or similar)
- **Python**: Python 3.8+ with pip
- **EVerest Framework**: Version 2024.3.0 or later
- **CMake**: Version 3.20+
- **Git**: For source installation

### Network Requirements
- **Connectivity**: Ethernet connection to WhiteBeet module
- **IP Configuration**: Static IP recommended for production
- **Firewall**: Port 15118 accessible for HCI communication

## EVerest Source Installation

### 1. Clone EVerest Repository

```bash
# Clone the main EVerest repository
git clone https://github.com/EVerest/everest-core.git
cd everest-core

# Switch to your development branch if needed
git checkout am/whitebeet_eth_driver  # or your branch name
```

### 2. Install Dependencies

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    python3-dev \
    git \
    libboost-all-dev \
    nodejs \
    npm \
    rsync

# Install Python dependencies for the module
pip3 install -r modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/requirements.txt
```

### 3. Build EVerest

```bash
# Create build directory
mkdir build
cd build

# Configure build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/everest

# Build EVerest (this includes PyWhiteBeetEthDriver)
make -j$(nproc)

# Install to system (optional)
sudo make install
```

### 4. Verify Module Installation

```bash
# Check if the module is recognized
./build/run-scripts/run-manager.py --help

# List available modules (should include PyWhiteBeetEthDriver)
./build/run-scripts/run-manager.py --check config/config-py-whitebeet-test.yaml
```

## Docker Deployment

### 1. Create Dockerfile

```dockerfile
# Dockerfile for PyWhiteBeetEthDriver deployment
FROM ghcr.io/everest/everest-core:latest

# Install additional Python dependencies
COPY modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/requirements.txt /tmp/
RUN pip3 install -r /tmp/requirements.txt

# Copy PyWhiteBeetEthDriver module
COPY modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/ \
     /workspace/everest-core/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/

# Copy configuration files
COPY config/config-py-whitebeet.yaml /workspace/everest-core/config/
COPY config/config-py-whitebeet-test.yaml /workspace/everest-core/config/
COPY config/config-py-whitebeet-dev.yaml /workspace/everest-core/config/

# Create logs directory
RUN mkdir -p /workspace/everest-core/logs

# Set working directory
WORKDIR /workspace/everest-core

# Default command
CMD ["./build/run-scripts/run-manager.py", "--config", "config/config-py-whitebeet.yaml"]
```

### 2. Build Docker Image

```bash
# Build the Docker image
docker build -t everest-py-whitebeet:latest .

# Or use docker-compose
cat > docker-compose.yml << EOF
version: '3.8'
services:
  everest-py-whitebeet:
    build: .
    image: everest-py-whitebeet:latest
    container_name: everest-py-whitebeet
    network_mode: host  # For direct hardware access
    volumes:
      - ./logs:/workspace/everest-core/logs
      - ./config:/workspace/everest-core/config
    environment:
      - EVEREST_CONFIG=config/config-py-whitebeet.yaml
    restart: unless-stopped
EOF
```

### 3. Deploy with Docker

```bash
# Run directly
docker run -d \
    --name everest-py-whitebeet \
    --network host \
    -v $(pwd)/logs:/workspace/everest-core/logs \
    -v $(pwd)/config:/workspace/everest-core/config \
    everest-py-whitebeet:latest

# Or use docker-compose
docker-compose up -d
```

## Package Installation

### 1. Create Debian Package (Advanced)

```bash
# Create package structure
mkdir -p everest-py-whitebeet/DEBIAN
mkdir -p everest-py-whitebeet/opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver
mkdir -p everest-py-whitebeet/etc/everest/config

# Copy files
cp -r modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/* \
    everest-py-whitebeet/opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/

cp config/config-py-whitebeet*.yaml everest-py-whitebeet/etc/everest/config/

# Create control file
cat > everest-py-whitebeet/DEBIAN/control << EOF
Package: everest-py-whitebeet
Version: 1.0.0
Section: net
Priority: optional
Architecture: all
Depends: python3 (>= 3.8), python3-pip, everest-core (>= 2024.3.0)
Maintainer: Your Name <your.email@example.com>
Description: Python WhiteBeet Ethernet Driver for EVerest
 This package provides a Python implementation of the WhiteBeet
 Ethernet driver for the EVerest charging infrastructure framework.
EOF

# Build package
dpkg-deb --build everest-py-whitebeet
```

### 2. Install Package

```bash
# Install the package
sudo dpkg -i everest-py-whitebeet.deb

# Install dependencies if needed
sudo apt-get install -f
```

## Manual Installation

### 1. Copy Module Files

```bash
# Define installation paths
EVEREST_ROOT="/opt/everest"
MODULE_PATH="$EVEREST_ROOT/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver"
CONFIG_PATH="$EVEREST_ROOT/config"

# Create directories
sudo mkdir -p "$MODULE_PATH"
sudo mkdir -p "$CONFIG_PATH"
sudo mkdir -p "/var/log/everest"

# Copy module files
sudo cp modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/* "$MODULE_PATH/"

# Copy configuration files
sudo cp config/config-py-whitebeet*.yaml "$CONFIG_PATH/"

# Set permissions
sudo chmod +x "$MODULE_PATH/module.py"
sudo chmod +x "$MODULE_PATH/test_whitebeet.py"
```

### 2. Install Python Dependencies

```bash
# Install Python dependencies
sudo pip3 install -r "$MODULE_PATH/requirements.txt"

# Or use virtual environment
python3 -m venv /opt/everest/venv
source /opt/everest/venv/bin/activate
pip install -r "$MODULE_PATH/requirements.txt"
```

### 3. Create Systemd Service (Optional)

```bash
# Create systemd service file
sudo tee /etc/systemd/system/everest-py-whitebeet.service << EOF
[Unit]
Description=EVerest Python WhiteBeet Driver
After=network.target
Wants=network.target

[Service]
Type=simple
User=everest
Group=everest
WorkingDirectory=/opt/everest
ExecStart=/opt/everest/build/run-scripts/run-manager.py --config config/config-py-whitebeet.yaml
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Create everest user
sudo useradd -r -s /bin/false everest
sudo chown -R everest:everest /opt/everest
sudo chown -R everest:everest /var/log/everest

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable everest-py-whitebeet.service
sudo systemctl start everest-py-whitebeet.service
```

## Configuration and Deployment

### 1. Network Configuration

```bash
# Configure static IP for EVerest host
sudo tee /etc/netplan/01-everest.yaml << EOF
network:
  version: 2
  ethernets:
    eth0:  # Replace with your interface name
      addresses:
        - 192.168.1.10/24
      gateway4: 192.168.1.1
      nameservers:
        addresses: [8.8.8.8, 8.8.4.4]
EOF

sudo netplan apply
```

### 2. WhiteBeet Hardware Configuration

```bash
# Configure WhiteBeet module IP address
# This depends on your WhiteBeet configuration method
# Typically done via web interface or configuration tool

# Test connectivity
ping 192.168.1.100
telnet 192.168.1.100 15118
```

### 3. EVerest Configuration

```bash
# Edit configuration for your environment
sudo nano /opt/everest/config/config-py-whitebeet.yaml

# Key settings to adjust:
# - ip_address: WhiteBeet IP address
# - evse_id: Your EVSE identifier
# - max_current_*: Current limits for your installation
# - phases: Number of phases (1 or 3)
```

### 4. Firewall Configuration

```bash
# Allow EVerest communication
sudo ufw allow 15118/tcp
sudo ufw allow from 192.168.1.100
sudo ufw allow to 192.168.1.100

# For development, allow additional ports
sudo ufw allow 8080/tcp  # Web interface
sudo ufw allow 9090/tcp  # Metrics
```

## Verification and Testing

### 1. Standalone Module Test

```bash
# Test module communication without EVerest
cd /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver
python3 test_whitebeet.py

# Expected output:
# ✅ Testing WhiteBeet connection to 192.168.1.100:15118
# ✅ Connection successful
# ✅ Protocol test passed
```

### 2. EVerest Integration Test

```bash
# Run EVerest with test configuration
cd /opt/everest
./build/run-scripts/run-manager.py --config config/config-py-whitebeet-test.yaml

# Check logs
tail -f logs/py-whitebeet-test.log

# Expected messages:
# [INFO] PyWhiteBeetEthDriver: Module initialized
# [INFO] PyWhiteBeetEthDriver: Connected to WhiteBeet at 192.168.1.100:15118
# [INFO] EvseManager: All interfaces ready
```

### 3. Full System Test

```bash
# Run production configuration
./build/run-scripts/run-manager.py --config config/config-py-whitebeet.yaml

# Monitor system status
./build/run-scripts/ev-cli.py --monitor

# Test charging workflow:
# 1. Connect EV cable
# 2. Verify SLAC communication
# 3. Check ISO 15118 handshake
# 4. Monitor power flow
```

## Troubleshooting

### Common Installation Issues

#### 1. Module Not Found
```bash
# Check module registration
grep -r "PyWhiteBeetEthDriver" /opt/everest/modules/

# Verify CMakeLists.txt includes the module
cat /opt/everest/modules/HardwareDrivers/EVSE/CMakeLists.txt | grep PyWhiteBeet

# Rebuild if necessary
cd /opt/everest/build && make clean && make -j$(nproc)
```

#### 2. Python Dependencies Missing
```bash
# Install missing dependencies
pip3 install -r /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/requirements.txt

# Check Python path
python3 -c "import sys; print('\n'.join(sys.path))"

# Verify imports
python3 -c "import socket, threading, struct, time, json, logging"
```

#### 3. Permission Issues
```bash
# Fix file permissions
sudo chown -R everest:everest /opt/everest
sudo chmod +x /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/module.py

# Fix log directory permissions
sudo mkdir -p /var/log/everest
sudo chown everest:everest /var/log/everest
```

#### 4. Network Connectivity Issues
```bash
# Test network connectivity
ping 192.168.1.100
telnet 192.168.1.100 15118

# Check firewall
sudo ufw status
sudo iptables -L

# Test with netcat
nc -zv 192.168.1.100 15118
```

### Runtime Issues

#### 1. Connection Failures
```bash
# Check WhiteBeet hardware status
# Access WhiteBeet web interface at http://192.168.1.100

# Test with standalone script
cd /opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver
python3 test_whitebeet.py

# Check logs for detailed error messages
tail -f /var/log/everest/py-whitebeet.log
```

#### 2. SLAC Issues
```bash
# Check SLAC configuration
grep -A 10 "slac" /opt/everest/config/config-py-whitebeet.yaml

# Monitor SLAC messages
tail -f /var/log/everest/py-whitebeet.log | grep -i slac

# Test with different timeout values
```

#### 3. Power Measurement Issues
```bash
# Check powermeter configuration
grep -A 5 "powermeter" /opt/everest/config/config-py-whitebeet.yaml

# Monitor power readings
tail -f /var/log/everest/py-whitebeet.log | grep -i power

# Test with debug mode enabled
```

### Debug Tools

#### 1. Enable Debug Logging
```yaml
# In configuration file
logging:
  loggers:
    - logger: "py_whitebeet_driver"
      level: "debug"
```

#### 2. Monitor Network Traffic
```bash
# Capture network traffic
sudo tcpdump -i eth0 host 192.168.1.100 and port 15118

# Monitor with Wireshark for detailed analysis
```

#### 3. System Monitoring
```bash
# Monitor system resources
htop
iotop
netstat -tuln | grep 15118

# Check service status
sudo systemctl status everest-py-whitebeet.service
sudo journalctl -u everest-py-whitebeet.service -f
```

## Production Deployment Checklist

- [ ] Hardware properly connected and configured
- [ ] Network connectivity verified
- [ ] Static IP addresses configured
- [ ] Firewall rules configured
- [ ] EVerest built and installed
- [ ] PyWhiteBeetEthDriver module installed
- [ ] Configuration files customized
- [ ] Standalone tests passed
- [ ] Integration tests passed
- [ ] Logging configured
- [ ] Monitoring set up
- [ ] Backup procedures established
- [ ] Update procedures documented
- [ ] Emergency procedures documented

## Support and Maintenance

### Log Locations
- **EVerest Logs**: `/var/log/everest/` or `./logs/`
- **System Logs**: `/var/log/syslog`
- **Service Logs**: `journalctl -u everest-py-whitebeet.service`

### Configuration Files
- **Module Config**: `/opt/everest/config/config-py-whitebeet.yaml`
- **Module Source**: `/opt/everest/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/`

### Update Procedures
1. Stop EVerest service
2. Backup current configuration
3. Update module files
4. Test in development mode
5. Deploy to production
6. Verify operation

### Contact Information
- **EVerest Documentation**: https://everest.github.io/
- **WhiteBeet Hardware**: 8devices support
- **Module Issues**: Check GitHub repository issues

This installation guide provides comprehensive instructions for deploying the PyWhiteBeetEthDriver in various environments, from development to production.
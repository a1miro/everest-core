#!/bin/bash
# install_py_whitebeet.sh - Automated installation script for PyWhiteBeetEthDriver

set -e  # Exit on any error

# Configuration
EVEREST_ROOT="${EVEREST_ROOT:-/opt/everest}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$EVEREST_ROOT}"
CONFIG_DIR="${CONFIG_DIR:-$INSTALL_PREFIX/config}"
LOG_DIR="${LOG_DIR:-/var/log/everest}"
SERVICE_USER="${SERVICE_USER:-everest}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Check OS
    if ! command -v apt-get &> /dev/null; then
        log_error "This script is designed for Debian/Ubuntu systems"
        exit 1
    fi
    
    # Check Python
    if ! python3 --version | grep -q "Python 3.[8-9]\|Python 3.1[0-9]"; then
        log_error "Python 3.8+ is required"
        exit 1
    fi
    
    # Check available space (minimum 2GB)
    available_space=$(df / | awk 'NR==2{print $4}')
    if [ "$available_space" -lt 2000000 ]; then
        log_warn "Less than 2GB free space available"
    fi
    
    log_info "System requirements check passed"
}

# Install system dependencies
install_dependencies() {
    log_info "Installing system dependencies..."
    
    apt-get update
    apt-get install -y \
        build-essential \
        cmake \
        python3 \
        python3-pip \
        python3-dev \
        python3-venv \
        git \
        libboost-all-dev \
        nodejs \
        npm \
        rsync \
        curl \
        wget \
        net-tools \
        telnet \
        netcat \
        ufw
    
    log_info "System dependencies installed"
}

# Create system user
create_user() {
    log_info "Creating system user: $SERVICE_USER"
    
    if ! id "$SERVICE_USER" &>/dev/null; then
        useradd -r -s /bin/false -d "$EVEREST_ROOT" "$SERVICE_USER"
        log_info "User $SERVICE_USER created"
    else
        log_info "User $SERVICE_USER already exists"
    fi
}

# Create directories
create_directories() {
    log_info "Creating directories..."
    
    mkdir -p "$INSTALL_PREFIX"
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$LOG_DIR"
    mkdir -p "$INSTALL_PREFIX/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver"
    
    log_info "Directories created"
}

# Install EVerest (if not already installed)
install_everest() {
    if [ ! -d "$INSTALL_PREFIX/build" ]; then
        log_info "EVerest not found, installing from source..."
        
        cd /tmp
        git clone https://github.com/EVerest/everest-core.git
        cd everest-core
        
        mkdir build
        cd build
        
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        
        make -j$(nproc)
        make install
        
        log_info "EVerest installed"
    else
        log_info "EVerest already installed"
    fi
}

# Install PyWhiteBeetEthDriver module
install_module() {
    log_info "Installing PyWhiteBeetEthDriver module..."
    
    MODULE_DIR="$INSTALL_PREFIX/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver"
    
    # Copy module files (assuming they're in the current directory or source tree)
    if [ -f "module.py" ]; then
        # Install from current directory
        cp *.py "$MODULE_DIR/"
        cp *.yaml "$MODULE_DIR/"
        cp *.txt "$MODULE_DIR/"
        cp *.md "$MODULE_DIR/"
    elif [ -d "modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver" ]; then
        # Install from source tree
        cp modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver/* "$MODULE_DIR/"
    else
        log_error "PyWhiteBeetEthDriver source files not found"
        exit 1
    fi
    
    # Install Python dependencies
    if [ -f "$MODULE_DIR/requirements.txt" ]; then
        pip3 install -r "$MODULE_DIR/requirements.txt"
    fi
    
    # Copy configuration files
    if [ -d "config" ]; then
        cp config/config-py-whitebeet*.yaml "$CONFIG_DIR/"
    fi
    
    # Set permissions
    chmod +x "$MODULE_DIR/module.py"
    chmod +x "$MODULE_DIR/test_whitebeet.py"
    
    log_info "PyWhiteBeetEthDriver module installed"
}

# Update EVerest build configuration
update_build_config() {
    log_info "Updating EVerest build configuration..."
    
    CMAKE_FILE="$INSTALL_PREFIX/modules/HardwareDrivers/EVSE/CMakeLists.txt"
    
    if [ -f "$CMAKE_FILE" ]; then
        if ! grep -q "PyWhiteBeetEthDriver" "$CMAKE_FILE"; then
            # Add PyWhiteBeetEthDriver to CMakeLists.txt
            sed -i '/ev_add_module(WhiteBeetEthDriver)/a ev_add_module(PyWhiteBeetEthDriver)' "$CMAKE_FILE"
            log_info "Added PyWhiteBeetEthDriver to build configuration"
        else
            log_info "PyWhiteBeetEthDriver already in build configuration"
        fi
    else
        log_warn "EVerest CMakeLists.txt not found, skipping build config update"
    fi
}

# Create systemd service
create_service() {
    log_info "Creating systemd service..."
    
    cat > /etc/systemd/system/everest-py-whitebeet.service << EOF
[Unit]
Description=EVerest Python WhiteBeet Driver
After=network.target
Wants=network.target

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$INSTALL_PREFIX
ExecStart=$INSTALL_PREFIX/build/run-scripts/run-manager.py --config config/config-py-whitebeet.yaml
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal
Environment="PYTHONPATH=$INSTALL_PREFIX/modules"

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    log_info "Systemd service created"
}

# Configure firewall
configure_firewall() {
    log_info "Configuring firewall..."
    
    # Enable UFW if not already enabled
    ufw --force enable
    
    # Allow WhiteBeet communication
    ufw allow 15118/tcp
    ufw allow from 192.168.1.0/24 to any port 15118
    
    # Allow common EVerest ports
    ufw allow 8080/tcp   # Web interface
    ufw allow 9090/tcp   # Metrics
    
    log_info "Firewall configured"
}

# Set file permissions
set_permissions() {
    log_info "Setting file permissions..."
    
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_PREFIX"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$LOG_DIR"
    
    # Ensure executable permissions
    find "$INSTALL_PREFIX" -name "*.py" -exec chmod +x {} \;
    
    log_info "File permissions set"
}

# Test installation
test_installation() {
    log_info "Testing installation..."
    
    # Test Python dependencies
    python3 -c "import socket, threading, struct, time, json, logging" || {
        log_error "Python dependencies test failed"
        return 1
    }
    
    # Test module import
    cd "$INSTALL_PREFIX/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver"
    python3 -c "import sys; sys.path.insert(0, '.'); import module" || {
        log_error "Module import test failed"
        return 1
    }
    
    # Test configuration file
    if [ -f "$CONFIG_DIR/config-py-whitebeet-test.yaml" ]; then
        log_info "Configuration file found"
    else
        log_error "Configuration file not found"
        return 1
    fi
    
    log_info "Installation tests passed"
}

# Main installation function
main() {
    log_info "Starting PyWhiteBeetEthDriver installation..."
    
    check_root
    check_requirements
    install_dependencies
    create_user
    create_directories
    install_everest
    install_module
    update_build_config
    create_service
    configure_firewall
    set_permissions
    test_installation
    
    log_info "Installation completed successfully!"
    log_info ""
    log_info "Next steps:"
    log_info "1. Configure WhiteBeet hardware IP address"
    log_info "2. Edit configuration: $CONFIG_DIR/config-py-whitebeet.yaml"
    log_info "3. Test standalone: cd $INSTALL_PREFIX/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver && python3 test_whitebeet.py"
    log_info "4. Start service: systemctl start everest-py-whitebeet.service"
    log_info "5. Enable auto-start: systemctl enable everest-py-whitebeet.service"
    log_info ""
    log_info "Log files:"
    log_info "- Service logs: journalctl -u everest-py-whitebeet.service -f"
    log_info "- Application logs: tail -f $LOG_DIR/py-whitebeet.log"
}

# Command line options
case "${1:-install}" in
    "install")
        main
        ;;
    "test")
        test_installation
        ;;
    "uninstall")
        log_info "Stopping service..."
        systemctl stop everest-py-whitebeet.service || true
        systemctl disable everest-py-whitebeet.service || true
        rm -f /etc/systemd/system/everest-py-whitebeet.service
        systemctl daemon-reload
        
        log_info "Removing files..."
        rm -rf "$INSTALL_PREFIX/modules/HardwareDrivers/EVSE/PyWhiteBeetEthDriver"
        rm -f "$CONFIG_DIR/config-py-whitebeet*.yaml"
        
        log_info "Uninstallation completed"
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [install|test|uninstall|help]"
        echo ""
        echo "Commands:"
        echo "  install    - Install PyWhiteBeetEthDriver (default)"
        echo "  test       - Test existing installation"
        echo "  uninstall  - Remove PyWhiteBeetEthDriver"
        echo "  help       - Show this help message"
        echo ""
        echo "Environment variables:"
        echo "  EVEREST_ROOT  - EVerest installation directory (default: /opt/everest)"
        echo "  SERVICE_USER  - System user for service (default: everest)"
        echo "  LOG_DIR       - Log directory (default: /var/log/everest)"
        ;;
    *)
        log_error "Unknown command: $1"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac
# Python EVerest Module Development Guide

This guide explains how to create and implement EVerest modules using Python, with the PyWhiteBeetEthDriver as a complete reference implementation.

## Overview

Python modules in EVerest allow you to implement hardware drivers and other functionality using Python instead of C++. This provides advantages for rapid development, easier integration with Python libraries, and simpler debugging.

## Table of Contents

1. [Module Structure](#module-structure)
2. [Manifest Configuration](#manifest-configuration)
3. [Core Implementation](#core-implementation)
4. [Interface Implementation](#interface-implementation)
5. [Communication Patterns](#communication-patterns)
6. [Testing and Debugging](#testing-and-debugging)
7. [Build Integration](#build-integration)
8. [Best Practices](#best-practices)

## Module Structure

A Python EVerest module requires the following files:

```
MyPythonModule/
├── manifest.yaml           # EVerest module configuration
├── module.py               # Main module implementation (required)
├── requirements.txt        # Python dependencies (optional)
├── README.md              # Module documentation (recommended)
├── CMakeLists.txt         # Build configuration (usually empty for Python)
└── [other_files].py       # Additional Python modules as needed
```

### Key Files

- **`manifest.yaml`**: Defines interfaces, configuration, and metadata
- **`module.py`**: Contains the main module class and EVerest integration
- **`requirements.txt`**: Lists Python package dependencies
- **Additional `.py` files**: Custom libraries and utilities

## Manifest Configuration

The manifest.yaml defines your module's interfaces and configuration:

```yaml
description: Your module description here

# Interfaces your module provides
provides:
  interface_name:
    interface: interface_type
    description: Interface description

# Configuration parameters
config:
  parameter_name:
    description: Parameter description
    type: string|integer|number|boolean|object|array
    default: default_value

# Module metadata
metadata:
  license: https://opensource.org/licenses/Apache-2.0
  authors:
    - Your Name
```

### Example: WhiteBeet Driver Manifest

```yaml
description: Python implementation of WhiteBeet Ethernet driver for ISO 15118/SLAC

provides:
  evse_board_support:
    interface: evse_board_support
    description: EVSE board support interface
  slac:
    interface: slac
    description: SLAC interface for powerline communication
  # ... more interfaces

config:
  ip_address:
    description: IP address of the WhiteBeet module
    type: string
    default: "192.168.1.100"
  port:
    description: TCP port of the WhiteBeet module
    type: integer
    default: 15118
  # ... more config parameters
```

## Core Implementation

### Basic Module Structure

Every Python EVerest module follows this pattern:

```python
#!/usr/bin/env python3
import logging
from everest.framework import Module, RuntimeSession, log

class MyPythonModule:
    def __init__(self):
        # Initialize EVerest framework
        self._session = RuntimeSession()
        self._module = Module(self._session)
        
        # Update process name for logging
        log.update_process_name(self._module.info.id)
        
        # Get module configuration
        self._setup = self._module.say_hello()
        self._config = self._setup.configs.module
        
        # Setup interface implementations
        self._setup_implementations()
        
        # Initialize module
        self._module.init_done(self._ready)
    
    def _setup_implementations(self):
        """Setup all interface command implementations"""
        for interface_name in self._module.implementations:
            for cmd in self._module.implementations[interface_name].commands:
                self._module.implement_command(
                    interface_name, cmd, 
                    getattr(self, f'_{interface_name}_{cmd}')
                )
    
    def _ready(self):
        """Called when module is ready"""
        log.info("Module ready")
    
    # Interface command implementations
    def _interface_name_command_name(self, args):
        """Implementation of interface command"""
        # Your implementation here
        return result

# Entry point
def main():
    module = MyPythonModule()
    # Your main loop here

if __name__ == "__main__":
    main()
```

### EVerest Framework Integration

Key components of EVerest integration:

1. **RuntimeSession**: Manages the module's runtime environment
2. **Module**: Provides the main interface to the EVerest framework
3. **Configuration**: Access to module configuration parameters
4. **Interface Implementation**: Register handlers for interface commands
5. **Variable Publishing**: Send data and events to other modules

```python
# Access configuration
ip_address = self._config['ip_address']
port = self._config['port']

# Publish variables
self._module.publish_variable('interface_name', 'variable_name', data)

# Implement commands
def _interface_command(self, args):
    # args contains the command parameters
    # Return appropriate response
    return True  # or data structure
```

## Interface Implementation

### Command Implementation Pattern

For each interface your module provides, implement all required commands:

```python
def _interface_name_command_name(self, args: Dict[str, Any]) -> Any:
    """
    Implementation of interface command
    
    Args:
        args: Dictionary containing command parameters
        
    Returns:
        Command result (type depends on interface specification)
    """
    try:
        # Extract parameters
        param1 = args.get('param1')
        param2 = args.get('param2')
        
        # Perform operation
        result = self._do_something(param1, param2)
        
        # Publish events if needed
        self._module.publish_variable('interface_name', 'event', {
            'status': 'success',
            'data': result
        })
        
        return result
        
    except Exception as e:
        log.error(f"Command failed: {e}")
        return False
```

### Variable Publishing

Publish data and events to the EVerest framework:

```python
# Publish telemetry data
self._module.publish_variable('powermeter', 'powermeter', {
    'timestamp': time.time(),
    'power_W': {'total': 1500.0, 'L1': 500.0, 'L2': 500.0, 'L3': 500.0},
    'voltage_V': {'L1': 230.0, 'L2': 230.0, 'L3': 230.0},
    'current_A': {'L1': 6.5, 'L2': 6.5, 'L3': 6.5}
})

# Publish events
self._module.publish_variable('evse_board_support', 'event', {
    'event': 'enabled'
})

# Publish state changes
self._module.publish_variable('slac', 'state', 'MATCHING')
```

## Communication Patterns

### Hardware Communication

For hardware communication, implement a separate communication library:

```python
# Separate communication module (e.g., hardware_comm.py)
class HardwareCommunication:
    def __init__(self, config):
        self.ip_address = config['ip_address']
        self.port = config['port']
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to hardware"""
        # Implementation here
        return True
    
    def send_command(self, command: str, data: bytes = b'') -> bool:
        """Send command to hardware"""
        # Implementation here
        return True
    
    def register_callback(self, callback):
        """Register callback for incoming data"""
        # Implementation here
        pass

# Use in main module
class MyModule:
    def __init__(self):
        # ... EVerest setup ...
        self._hardware = HardwareCommunication(self._config)
        self._hardware.register_callback(self._handle_hardware_data)
    
    def _handle_hardware_data(self, data):
        """Handle incoming hardware data"""
        # Process data and publish to EVerest
        self._module.publish_variable('interface_name', 'data', processed_data)
```

### Threading Patterns

For asynchronous operations, use threading:

```python
import threading
import time

class MyModule:
    def __init__(self):
        # ... setup ...
        self._running = False
        self._worker_thread = None
    
    def start(self):
        """Start the module"""
        self._running = True
        self._worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
        self._worker_thread.start()
    
    def stop(self):
        """Stop the module"""
        self._running = False
        if self._worker_thread:
            self._worker_thread.join(timeout=5.0)
    
    def _worker_loop(self):
        """Background worker thread"""
        while self._running:
            try:
                # Do periodic work
                data = self._hardware.get_data()
                if data:
                    self._module.publish_variable('interface', 'data', data)
                
                time.sleep(1.0)
            except Exception as e:
                log.error(f"Worker error: {e}")
                time.sleep(5.0)  # Wait before retry
```

## Testing and Debugging

### Standalone Testing

Create test scripts that can run independently of EVerest:

```python
#!/usr/bin/env python3
"""Standalone test for hardware communication"""

import sys
import logging
from pathlib import Path

# Add module path
sys.path.insert(0, str(Path(__file__).parent))

from my_hardware_comm import HardwareCommunication

def test_hardware():
    """Test hardware communication"""
    config = {
        'ip_address': '192.168.1.100',
        'port': 12345
    }
    
    hardware = HardwareCommunication(config)
    
    if hardware.connect():
        print("✅ Connection successful")
        
        # Test commands
        if hardware.send_command('test'):
            print("✅ Command sent successfully")
        else:
            print("❌ Command failed")
            
        hardware.disconnect()
    else:
        print("❌ Connection failed")

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    test_hardware()
```

### Debug Mode

Enable detailed logging for debugging:

```python
import logging

# Set up detailed logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

# Use EVerest logging in module
from everest.framework import log

class MyModule:
    def _interface_command(self, args):
        log.debug(f"Command called with args: {args}")
        try:
            result = self._do_work(args)
            log.info(f"Command completed successfully: {result}")
            return result
        except Exception as e:
            log.error(f"Command failed: {e}")
            raise
```

### Mock Testing

Create mocks for hardware when actual hardware isn't available:

```python
class MockHardware:
    """Mock hardware for testing"""
    
    def __init__(self, config):
        self.config = config
        self.connected = False
    
    def connect(self) -> bool:
        """Simulate connection"""
        self.connected = True
        return True
    
    def send_command(self, command: str) -> bool:
        """Simulate command sending"""
        if not self.connected:
            return False
        print(f"Mock: Sending command {command}")
        return True
    
    def get_data(self):
        """Return mock data"""
        return {
            'timestamp': time.time(),
            'value': 42.0,
            'status': 'ok'
        }

# Use mock in testing
if __name__ == "__main__":
    hardware = MockHardware({'ip': 'test', 'port': 1234})
    # Test with mock hardware
```

## Build Integration

### CMakeLists.txt

Python modules typically have minimal or empty CMakeLists.txt:

```cmake
# For Python modules, usually empty or minimal
# EVerest will handle Python module discovery

# Optional: Add custom installation rules
# install(FILES module.py DESTINATION ${CMAKE_INSTALL_PREFIX}/modules/MyModule/)
```

### Module Registration

Add your module to the parent CMakeLists.txt:

```cmake
# In modules/HardwareDrivers/EVSE/CMakeLists.txt
ev_add_module(PyWhiteBeetEthDriver)
```

### Python Dependencies

List dependencies in requirements.txt:

```txt
# External dependencies (if any)
requests>=2.25.0
pyserial>=3.5

# EVerest framework is provided by the environment
# Standard library modules don't need to be listed
```

## Best Practices

### Error Handling

```python
def _interface_command(self, args):
    """Robust error handling example"""
    try:
        # Validate input
        if 'required_param' not in args:
            log.error("Missing required parameter")
            return False
        
        # Perform operation with timeout
        result = self._hardware_operation(args, timeout=5.0)
        
        # Validate result
        if result is None:
            log.warning("Operation returned no result")
            return False
        
        return result
        
    except TimeoutError:
        log.error("Operation timed out")
        return False
    except ValueError as e:
        log.error(f"Invalid parameter: {e}")
        return False
    except Exception as e:
        log.error(f"Unexpected error: {e}")
        # Optionally re-raise for debugging
        return False
```

### Configuration Validation

```python
def __init__(self):
    # ... EVerest setup ...
    
    # Validate configuration
    self._validate_config()

def _validate_config(self):
    """Validate module configuration"""
    required_params = ['ip_address', 'port']
    
    for param in required_params:
        if param not in self._config:
            raise ValueError(f"Missing required config parameter: {param}")
    
    # Validate IP address format
    import ipaddress
    try:
        ipaddress.ip_address(self._config['ip_address'])
    except ValueError:
        raise ValueError(f"Invalid IP address: {self._config['ip_address']}")
    
    # Validate port range
    port = self._config['port']
    if not 1 <= port <= 65535:
        raise ValueError(f"Invalid port number: {port}")
```

### Resource Management

```python
class MyModule:
    def __init__(self):
        # ... setup ...
        self._resources = []
    
    def _acquire_resource(self, resource):
        """Acquire and track resources"""
        self._resources.append(resource)
        return resource
    
    def _cleanup(self):
        """Clean up all resources"""
        for resource in reversed(self._resources):
            try:
                resource.close()
            except Exception as e:
                log.warning(f"Error closing resource: {e}")
        self._resources.clear()
    
    def __del__(self):
        """Ensure cleanup on destruction"""
        self._cleanup()
```

### State Management

```python
from enum import Enum

class ModuleState(Enum):
    INITIALIZING = "initializing"
    READY = "ready"
    CONNECTED = "connected"
    ERROR = "error"

class MyModule:
    def __init__(self):
        # ... setup ...
        self._state = ModuleState.INITIALIZING
    
    def _set_state(self, new_state: ModuleState):
        """Set module state and publish event"""
        if self._state != new_state:
            old_state = self._state
            self._state = new_state
            
            log.info(f"State changed: {old_state.value} -> {new_state.value}")
            
            # Publish state change
            self._module.publish_variable('status', 'state', new_state.value)
    
    def _ready(self):
        """Called when module is ready"""
        self._set_state(ModuleState.READY)
```

## Reference Implementation

The **PyWhiteBeetEthDriver** provides a complete reference implementation demonstrating:

- ✅ **Complete EVerest Integration**: All framework components properly used
- ✅ **Multiple Interface Support**: Six different interfaces implemented
- ✅ **Hardware Communication**: TCP/IP protocol implementation
- ✅ **Threading**: Asynchronous message handling
- ✅ **Error Handling**: Comprehensive error management
- ✅ **Testing**: Standalone test capabilities
- ✅ **Documentation**: Complete documentation and examples

Study the PyWhiteBeetEthDriver implementation for practical examples of all these patterns and practices.

## Conclusion

Python modules in EVerest provide a powerful way to implement hardware drivers and other functionality with the flexibility and ease of Python development. Follow the patterns shown in this guide and the PyWhiteBeetEthDriver reference implementation to create robust, maintainable Python modules for your EVerest deployment.

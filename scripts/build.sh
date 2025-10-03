#!/bin/bash

# Smart Tile Lighting System Build Script
# This script builds both coordinator and node firmware

set -e

echo "Smart Tile Lighting System Build Script"
echo "======================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    print_error "PlatformIO is not installed. Please install it first."
    exit 1
fi

# Build coordinator
print_status "Building coordinator firmware..."
cd ../coordinator
if pio run; then
    print_status "Coordinator build successful"
else
    print_error "Coordinator build failed"
    exit 1
fi

# Build node
print_status "Building node firmware..."
cd ../node
if pio run; then
    print_status "Node build successful"
else
    print_error "Node build failed"
    exit 1
fi

print_status "All builds completed successfully!"
print_status "Firmware files are located in:"
print_status "  Coordinator: coordinator/.pio/build/esp32-s3-devkitc-1/firmware.bin"
print_status "  Node: node/.pio/build/esp32-c3-devkitm-1/firmware.bin"

# Optional: Create release package
if [ "$1" = "--package" ]; then
    print_status "Creating release package..."
    cd ..
    RELEASE_DIR="smart_tile_release_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$RELEASE_DIR"
    
    cp coordinator/.pio/build/esp32-s3-devkitc-1/firmware.bin "$RELEASE_DIR/coordinator_firmware.bin"
    cp node/.pio/build/esp32-c3-devkitm-1/firmware.bin "$RELEASE_DIR/node_firmware.bin"
    cp ProductRequirementDocument.md "$RELEASE_DIR/"
    
    # Create README for release
    cat > "$RELEASE_DIR/README.md" << EOF
# Smart Tile Lighting System - Release Package

## Firmware Files
- \`coordinator_firmware.bin\` - ESP32-S3 coordinator firmware
- \`node_firmware.bin\` - ESP32-C3 node firmware

## Installation Instructions

### Coordinator (ESP32-S3)
1. Connect ESP32-S3 to computer via USB
2. Use PlatformIO or esptool to flash the coordinator firmware
3. Configure WiFi and MQTT settings via serial console

### Node (ESP32-C3)
1. Connect ESP32-C3 to computer via USB
2. Use PlatformIO or esptool to flash the node firmware
3. Follow pairing procedure with coordinator

## Configuration
See ProductRequirementDocument.md for detailed configuration options.

## Support
For issues and questions, refer to the project documentation.
EOF

    print_status "Release package created: $RELEASE_DIR"
fi




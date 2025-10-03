#!/bin/bash

# Smart Tile Lighting System Upload Script
# This script uploads firmware to ESP32 devices

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [coordinator|node] [port]"
    echo ""
    echo "Arguments:"
    echo "  coordinator  Upload coordinator firmware to ESP32-S3"
    echo "  node         Upload node firmware to ESP32-C3"
    echo "  port         Serial port (optional, auto-detected if not specified)"
    echo ""
    echo "Examples:"
    echo "  $0 coordinator"
    echo "  $0 node /dev/ttyUSB0"
    echo "  $0 coordinator COM3"
}

# Check arguments
if [ $# -lt 1 ]; then
    show_usage
    exit 1
fi

DEVICE_TYPE=$1
PORT=$2

# Detect port if not specified
if [ -z "$PORT" ]; then
    print_status "Auto-detecting serial port..."
    
    # Try to detect ESP32 devices
    if command -v lsusb &> /dev/null; then
        # Linux
        PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1)
    elif command -v system_profiler &> /dev/null; then
        # macOS
        PORT=$(ls /dev/cu.usbserial* /dev/cu.usbmodem* 2>/dev/null | head -1)
    else
        # Windows or fallback
        PORT="auto"
    fi
    
    if [ -z "$PORT" ]; then
        print_warning "Could not auto-detect port. Using 'auto'"
        PORT="auto"
    else
        print_status "Detected port: $PORT"
    fi
fi

# Upload based on device type
case $DEVICE_TYPE in
    coordinator)
        print_status "Uploading coordinator firmware..."
        cd ../coordinator
        
        if [ "$PORT" != "auto" ]; then
            pio run --target upload --upload-port "$PORT"
        else
            pio run --target upload
        fi
        
        if [ $? -eq 0 ]; then
            print_status "Coordinator firmware uploaded successfully!"
            print_status "Opening serial monitor..."
            if [ "$PORT" != "auto" ]; then
                pio device monitor --port "$PORT"
            else
                pio device monitor
            fi
        else
            print_error "Coordinator upload failed!"
            exit 1
        fi
        ;;
        
    node)
        print_status "Uploading node firmware..."
        cd ../node
        
        if [ "$PORT" != "auto" ]; then
            pio run --target upload --upload-port "$PORT"
        else
            pio run --target upload
        fi
        
        if [ $? -eq 0 ]; then
            print_status "Node firmware uploaded successfully!"
            print_status "Opening serial monitor..."
            if [ "$PORT" != "auto" ]; then
                pio device monitor --port "$PORT"
            else
                pio device monitor
            fi
        else
            print_error "Node upload failed!"
            exit 1
        fi
        ;;
        
    *)
        print_error "Invalid device type: $DEVICE_TYPE"
        show_usage
        exit 1
        ;;
esac




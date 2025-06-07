#!/bin/bash

# Test script for AESD socket with IOCTL support

SERVER_HOST="localhost"
SERVER_PORT="9000"
DEVICE_PATH="/dev/aesdchar"

echo "Testing AESD Socket with IOCTL support"

# Function to send data to socket and capture response
send_to_socket() {
    local data="$1"
    echo "Sending: $data"
    echo -n "$data" | nc $SERVER_HOST $SERVER_PORT
    echo ""
}

# Start by writing some test data
echo "=== Writing test data ==="
send_to_socket "Hello World"
send_to_socket "This is command 2"
send_to_socket "Command 3 here"
send_to_socket "Final command"

echo ""
echo "=== Testing IOCTL commands ==="

# Test IOCTL command - seek to command 1, offset 5
echo "Testing AESDCHAR_IOCSEEKTO:1,5"
send_to_socket "AESDCHAR_IOCSEEKTO:1,5"

echo ""
# Test IOCTL command - seek to command 0, offset 0
echo "Testing AESDCHAR_IOCSEEKTO:0,0"
send_to_socket "AESDCHAR_IOCSEEKTO:0,0"

echo ""
# Test invalid IOCTL command
echo "Testing invalid AESDCHAR_IOCSEEKTO:10,0"
send_to_socket "AESDCHAR_IOCSEEKTO:10,0"

echo ""
echo "Test completed"

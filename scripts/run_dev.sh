#!/bin/bash
# scripts/run_dev.sh

# Set up development environment
export VIZ1090_DEV=1

# Build in development mode
go build -tags dev -o viz1090_dev ./cmd/viz1090

# Start Beast format simulator if requested
if [ "$1" == "--sim" ]; then
    echo "Starting Beast simulator on port 30005..."
    go run ./cmd/beastsim/main.go &
    SIM_PID=$!

    # Kill simulator when script exits
    trap "kill $SIM_PID" EXIT

    # Give simulator time to start
    sleep 1
fi

# Run with developer-friendly settings
./viz1090_dev --dev --server=localhost --port=30005
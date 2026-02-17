#!/bin/bash

# Profile the n-body simulation using DTrace and generate a flame graph

set -e

APP_PATH="./build/raylib_app"
DURATION=10  # seconds to profile
FLAMEGRAPH_DIR="/tmp/FlameGraph"
OUTPUT_DIR="./profile_output"

mkdir -p "$OUTPUT_DIR"

echo "Starting n-body simulation..."
# Start the app in the background
$APP_PATH &
APP_PID=$!

# Give it a moment to initialize
sleep 1

echo "Profiling for ${DURATION} seconds using DTrace (PID: $APP_PID)..."

# Use DTrace to profile the process
# This requires root permissions, so we'll use sudo
sudo dtrace -x ustackframes=100 -n "profile-997 /pid == $APP_PID/ { @[ustack()] = count(); }" -o "$OUTPUT_DIR/dtrace.out" &
DTRACE_PID=$!

# Wait for the specified duration
sleep $DURATION

# Stop DTrace
echo "Stopping profiler..."
sudo kill -INT $DTRACE_PID 2>/dev/null || true
sleep 1

# Kill the app
echo "Killing the app..."
kill $APP_PID 2>/dev/null || true
wait $APP_PID 2>/dev/null || true

echo "Converting DTrace data to flame graph format..."
# Process the DTrace output
$FLAMEGRAPH_DIR/stackcollapse.pl "$OUTPUT_DIR/dtrace.out" > "$OUTPUT_DIR/collapsed.txt"

echo "Generating flame graph..."
$FLAMEGRAPH_DIR/flamegraph.pl "$OUTPUT_DIR/collapsed.txt" > "$OUTPUT_DIR/flamegraph.svg"

echo ""
echo "✓ Flame graph generated: $OUTPUT_DIR/flamegraph.svg"
echo ""
echo "To view it, run: open $OUTPUT_DIR/flamegraph.svg"

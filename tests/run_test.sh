#!/bin/sh
set -e

echo "=== Running Standalone Unit Tests ==="
./test_ipc

echo ""
echo "=== Running End-to-End Integration Test with Parent Process ==="

# Clean previous csv if any
rm -f results/observations.csv

# Start bison_rescue in background for 3 seconds with 2 expeditions
./bison_rescue 2 3 &
MAIN_PID=$!

# Wait briefly for main to initialize message queue and create results/observations.csv
sleep 0.5

# Send sample observations into queue using mock_sender
./mock_sender 3

# Wait for parent process to finish its session
wait $MAIN_PID

echo ""
echo "=== Checking results/observations.csv ==="
if [ ! -f results/observations.csv ]; then
    echo "ERROR: results/observations.csv was not created!"
    exit 1
fi

cat results/observations.csv
echo ""

# Verify CSV Header
HEADER=$(head -n 1 results/observations.csv)
EXPECTED_HEADER="Timestamp,Territory,Observer,Count,AgeGroup"
if [ "$HEADER" != "$EXPECTED_HEADER" ]; then
    echo "ERROR: Unexpected CSV header: '$HEADER', expected: '$EXPECTED_HEADER'"
    exit 1
fi

# Verify observation lines
LINES_COUNT=$(wc -l < results/observations.csv | tr -d ' ')
if [ "$LINES_COUNT" -lt 4 ]; then
    echo "ERROR: Expected at least 4 lines (1 header + 3 data lines), got $LINES_COUNT"
    exit 1
fi

echo "=== ALL INTEGRATION TESTS PASSED SUCCESSFULLY! ==="

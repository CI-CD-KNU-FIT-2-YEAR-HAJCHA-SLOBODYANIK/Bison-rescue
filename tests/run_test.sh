#!/bin/sh
set -e

echo "=== 1. Running Standalone IPC Unit Tests ==="
./test_ipc

echo ""
echo "=== 2. Testing Argument Parsing and Full Data Flow (-n 3 -t 4) ==="

# Clean previous csv if any
rm -f results/observations.csv

# Run bison_rescue with -n 3 -t 4
./bison_rescue -n 3 -t 4

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
    echo "ERROR: Expected at least 4 lines (1 header + multiple observation lines), got $LINES_COUNT"
    exit 1
fi

echo "=== 3. Testing Mock Sender with Parent Process ==="
rm -f results/observations.csv
./bison_rescue -n 1 -t 3 &
PID=$!
sleep 0.5
./mock_sender 2
wait $PID

if [ ! -f results/observations.csv ]; then
    echo "ERROR: results/observations.csv was not created during mock test!"
    exit 1
fi

echo "=== ALL INTEGRATION AND FULL DATA FLOW TESTS PASSED SUCCESSFULLY! ==="

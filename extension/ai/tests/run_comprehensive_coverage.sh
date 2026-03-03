#!/bin/bash
# Comprehensive coverage test - single session approach

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build"
EXT_DIR="${BUILD_DIR}/test/extension"
SQL_FILE="${SCRIPT_DIR}/comprehensive_coverage_test.sql"

echo "=== Comprehensive Coverage Test ==="
echo "Strategy: Single-session test to avoid .gcda overwrite"
echo ""

# Clean old coverage data
echo "Cleaning old coverage data..."
rm -f ${EXT_DIR}/*.gcda
rm -f ${BUILD_DIR}/**/*.gcda
find ${BUILD_DIR} -name "*.gcda" -delete 2>/dev/null || true

# Run comprehensive test in single session
echo "Running comprehensive test in single DuckDB session..."
${BUILD_DIR}/duckdb -unsigned < "${SQL_FILE}" > /tmp/duckdb_test_output.log 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Test execution completed"
else
    echo "⚠ Test execution had issues (see /tmp/duckdb_test_output.log)"
fi

# Check for generated .gcda files
echo ""
echo "Checking for .gcda files:"
find ${EXT_DIR} -name "*.gcda" -exec ls -la {} \; 2>/dev/null || echo "No .gcda files found"

# Generate coverage report
echo ""
echo "=== Coverage Report ==="
cd /Users/yp1017/development/daft-duckdb-multimodal/duckdb

# Find all .gcno files and generate coverage
for gcno in ${EXT_DIR}/ai.duckdb_extension-*.gcno; do
    if [ -f "$gcno" ]; then
        basename_gcno=$(basename "$gcno")
        echo ""
        echo "Analyzing: ${basename_gcno}"
        gcov -o "${EXT_DIR}" "$gcno" 2>/dev/null || true
    fi
done

# Parse and summarize coverage
echo ""
echo "=== Summary ==="
for gcov in *.gcov; do
    if [ -f "$gcov" ]; then
        echo "$gcov:"
        grep -E "^(File|Lines|Functions)" "$gcov" | head -5
    fi
done | head -30

# Calculate total coverage
echo ""
echo "Calculating total coverage..."
python3 - << 'EOF'
import re
import os

total_lines = 0
executed_lines = 0

for file in os.listdir('.'):
    if file.endswith('.gcov'):
        with open(file, 'r') as f:
            content = f.read()
            # Extract line counts from gcov summary
            match = re.search(r'Lines executed:(\d+\.\d+)% of (\d+)', content)
            if match:
                percent = float(match.group(1))
                lines = int(match.group(2))
                executed = int(lines * percent / 100)
                total_lines += lines
                executed_lines += executed
                print(f"{file}: {percent}% of {lines} lines")

if total_lines > 0:
    total_percent = (executed_lines / total_lines) * 100
    print(f"\n{'='*50}")
    print(f"Total Coverage: {total_percent:.2f}% ({executed_lines}/{total_lines} lines)")
    print(f"{'='*50}")
else:
    print("No coverage data found")
EOF

#!/usr/bin/env python3
"""
DuckDB AI Extension CLI Test Script

Tests the ai_filter function via DuckDB CLI subprocess.
This works around Python duckdb library ABI compatibility issues.

Usage:
    python3 test_ai_cli.py
"""

import subprocess
import sys
import os
from pathlib import Path

# Configuration
DUCKDB_BINARY = Path(__file__).parent.parent.parent / "build" / "duckdb"
EXTENSION_PATH = Path(__file__).parent.parent.parent / "build" / "test" / "extension" / "ai.duckdb_extension"


def run_query(sql: str) -> tuple[int, str, str]:
    """Run a query via DuckDB CLI and return (exit_code, stdout, stderr)."""
    cmd = [str(DUCKDB_BINARY), "-unsigned", "-c", sql]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def test_extension_load():
    """Test 1: Extension can be loaded."""
    print("Test 1: Extension Loading")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}';"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Extension loading failed")
        print(f"Error: {stderr}")
        return False

    print("✅ PASSED: Extension loaded successfully")
    print()
    return True


def test_single_call():
    """Test 2: Single ai_filter() call returns correct value."""
    print("Test 2: Single Function Call")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT ai_filter() AS score;"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    if "0.42" not in stdout:
        print(f"❌ FAILED: Expected 0.42 in output")
        print(f"Got: {stdout}")
        return False

    print("✅ PASSED: Function returns correct value")
    print(f"Output:\n{stdout}")
    print()
    return True


def test_multi_row():
    """Test 3: Multi-row execution works correctly."""
    print("Test 3: Multi-Row Execution")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT ai_filter() AS score FROM range(5);"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Multi-row query failed")
        print(f"Error: {stderr}")
        return False

    # Check that we have 5 data rows (excluding header, type row, and separator lines)
    lines = stdout.strip().split('\n')
    # Count lines that start with │ and contain a digit (data rows)
    data_lines = [l for l in lines if l.startswith('│') and any(c.isdigit() for c in l)]

    if len(data_lines) != 5:
        print(f"❌ FAILED: Expected 5 data rows, got {len(data_lines)}")
        print(f"Output:\n{stdout}")
        return False

    # Check all values are 0.42
    for line in data_lines:
        if "0.42" not in line:
            print(f"❌ FAILED: Expected 0.42 in row: {line}")
            return False

    print("✅ PASSED: Multi-row execution works correctly")
    print(f"Output:\n{stdout}")
    print()
    return True


def test_function_registration():
    """Test 4: Function is registered in duckdb_functions()."""
    print("Test 4: Function Registration Verification")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT function_name FROM duckdb_functions() WHERE function_name = 'ai_filter';"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Function registration query failed")
        print(f"Error: {stderr}")
        return False

    if "ai_filter" not in stdout:
        print(f"❌ FAILED: ai_filter not found in duckdb_functions()")
        print(f"Output:\n{stdout}")
        return False

    print("✅ PASSED: Function is properly registered")
    print(f"Output:\n{stdout}")
    print()
    return True


def test_where_clause():
    """Test 5: ai_filter works in WHERE clause."""
    print("Test 5: WHERE Clause Integration")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT x FROM range(3) t(x) WHERE ai_filter() > 0.0;"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: WHERE clause query failed")
        print(f"Error: {stderr}")
        return False

    # Should return all 3 rows since 0.42 > 0.0
    if "0" not in stdout or "1" not in stdout or "2" not in stdout:
        print(f"❌ FAILED: Expected all 3 rows (0, 1, 2)")
        print(f"Output:\n{stdout}")
        return False

    print("✅ PASSED: WHERE clause filtering works")
    print(f"Output:\n{stdout}")
    print()
    return True


def main():
    """Run all tests."""
    print("=" * 60)
    print("DuckDB AI Extension CLI Test Suite")
    print("=" * 60)
    print(f"DuckDB Binary: {DUCKDB_BINARY}")
    print(f"Extension: {EXTENSION_PATH}")
    print(f"Extension Size: {EXTENSION_PATH.stat().st_size / 1024 / 1024:.1f} MB")
    print("=" * 60)
    print()

    # Check prerequisites
    if not DUCKDB_BINARY.exists():
        print(f"❌ ERROR: DuckDB binary not found at {DUCKDB_BINARY}")
        return 1

    if not EXTENSION_PATH.exists():
        print(f"❌ ERROR: Extension not found at {EXTENSION_PATH}")
        return 1

    # Run tests
    tests = [
        test_extension_load,
        test_single_call,
        test_multi_row,
        test_function_registration,
        test_where_clause,
    ]

    passed = 0
    failed = 0

    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"❌ EXCEPTION: {e}")
            failed += 1

    # Summary
    print("=" * 60)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("=" * 60)

    if failed == 0:
        print("✅ All tests passed!")
        return 0
    else:
        print("❌ Some tests failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())

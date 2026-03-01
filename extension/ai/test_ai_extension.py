#!/usr/bin/env python3
import duckdb
import sys
import os

def test_ai_filter():
    print("=== DuckDB AI Extension Test ===")
    print()

    con = duckdb.connect(':memory:')

    script_dir = os.path.dirname(os.path.abspath(__file__))
    ext_path = os.path.join(script_dir, '../build/extension/ai.duckdb_extension')
    print(f"Loading extension from: {ext_path}")

    try:
        con.execute(f"LOAD '{ext_path}'")
        print("Extension loaded successfully")
        print()
    except Exception as e:
        print(f"Failed to load extension: {e}")
        return False

    print("Test 1: Creating test table")
    con.execute("CREATE TABLE images AS SELECT id, 'image_' || id::VARCHAR || '.png'::BLOB AS image_data FROM range(5) t(id)")
    print("Table created")
    print()

    print("Test 2: Calling ai_filter function")
    result = con.execute("SELECT id, ai_filter(image_data, 'a cat', 'clip') AS score FROM images ORDER BY id").fetchall()

    print("Results:")
    for row in result:
        print(f"  ID {row[0]}: score = {row[1]:.4f}")
    print()

    if len(result) != 5:
        print(f"ERROR: Expected 5 rows, got {len(result)}")
        return False

    for row in result:
        score = row[1]
        if score < 0.0 or score > 1.0:
            print(f"ERROR: Score {score} not in range [0.0, 1.0]")
            return False

    print("All scores in valid range [0.0, 1.0]")
    print()

    print("Test 3: Using ai_filter in WHERE clause")
    result = con.execute("SELECT COUNT(*) AS count FROM images WHERE ai_filter(image_data, 'a cat', 'clip') > 0.5").fetchone()
    print(f"Rows with score > 0.5: {result[0]}")
    print()

    print("=== All Tests Passed! ===")
    return True

if __name__ == '__main__':
    try:
        success = test_ai_filter()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"Test failed with exception: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

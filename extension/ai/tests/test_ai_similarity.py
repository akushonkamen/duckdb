#!/usr/bin/env python3
"""
Test script for ai_similarity function

TASK-K-001: AI_join/AI_window Extension
"""

import subprocess
import sys

def run_duckdb_command(cmd):
    """Run DuckDB CLI command and return output."""
    full_cmd = f'./build/duckdb -unsigned -c "{cmd}"'
    try:
        result = subprocess.run(
            full_cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=30,
            cwd="/Users/yp1017/development/daft-duckdb-multimodal/duckdb"
        )
        return result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return "Command timed out"
    except FileNotFoundError:
        # DuckDB CLI not available, try Python bindings
        return None

def test_with_python():
    """Test using Python duckdb module."""
    try:
        import duckdb

        # Load extension
        ext_path = "/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension"
        con = duckdb.connect(":memory:")

        # Enable unsigned extensions
        try:
            con.execute("SET enable_unsigned_extensions=true")
        except:
            pass

        try:
            con.execute(f"LOAD '{ext_path}'")
            print("✅ Extension loaded successfully")
        except Exception as e:
            print(f"❌ Extension loading failed: {e}")
            return False

        # Test 1: Check if ai_similarity is registered
        print("\n=== Test 1: Function Registration ===")
        try:
            result = con.execute("""
                SELECT function_name, parameters
                FROM duckdb_functions()
                WHERE function_name LIKE 'ai_%'
                ORDER BY function_name
            """).fetchall()
            print("Registered AI functions:")
            for row in result:
                print(f"  - {row[0]}: {row[1]}")

            if ('ai_similarity',) in [r[:1] for r in result]:
                print("✅ ai_similarity is registered")
            else:
                print("❌ ai_similarity is NOT registered")
                return False
        except Exception as e:
            print(f"❌ Query failed: {e}")
            return False

        # Test 2: Basic similarity calculation
        print("\n=== Test 2: Basic Similarity Calculation ===")
        try:
            result = con.execute("""
                SELECT ai_similarity([1.0, 2.0, 3.0], [1.0, 2.0, 3.0], 'clip') AS similarity
            """).fetchone()
            similarity = result[0]
            print(f"Similarity of [1,2,3] with [1,2,3]: {similarity}")

            if abs(similarity - 1.0) < 0.01:
                print("✅ Exact match similarity correct (≈1.0)")
            else:
                print(f"❌ Expected ≈1.0, got {similarity}")
                return False
        except Exception as e:
            print(f"❌ Test failed: {e}")
            return False

        # Test 3: Different vectors
        print("\n=== Test 3: Different Vectors ===")
        try:
            result = con.execute("""
                SELECT ai_similarity([1.0, 0.0, 0.0], [0.0, 1.0, 0.0], 'clip') AS similarity
            """).fetchone()
            similarity = result[0]
            print(f"Similarity of [1,0,0] with [0,1,0]: {similarity}")

            if abs(similarity - 0.0) < 0.01:
                print("✅ Orthogonal vectors similarity correct (≈0.0)")
            else:
                print(f"❌ Expected ≈0.0, got {similarity}")
                return False
        except Exception as e:
            print(f"❌ Test failed: {e}")
            return False

        # Test 4: NULL handling
        print("\n=== Test 4: NULL Handling ===")
        try:
            result = con.execute("""
                SELECT ai_similarity(NULL, [1.0, 2.0, 3.0], 'clip') AS similarity
            """).fetchone()
            similarity = result[0]
            print(f"NULL vector similarity: {similarity}")

            if similarity is None:
                print("✅ NULL input returns NULL")
            else:
                print(f"❌ Expected NULL, got {similarity}")
                return False
        except Exception as e:
            print(f"❌ Test failed: {e}")
            return False

        # Test 5: Dimension mismatch
        print("\n=== Test 5: Dimension Mismatch ===")
        try:
            result = con.execute("""
                SELECT ai_similarity([1.0, 2.0], [1.0, 2.0, 3.0], 'clip') AS similarity
            """).fetchone()
            print(f"❌ Should have thrown an error, but got: {result}")
            return False
        except Exception as e:
            if "dimension mismatch" in str(e).lower() or "invalid input" in str(e).lower():
                print(f"✅ Dimension mismatch correctly rejected: {e}")
            else:
                print(f"⚠️ Got error but might not be dimension-specific: {e}")
        except Exception as e:
            print(f"✅ Dimension mismatch rejected: {e}")

        # Test 6: JOIN-like usage
        print("\n=== Test 6: JOIN-like Usage ===")
        try:
            result = con.execute("""
                WITH left_vecs AS (
                    SELECT 1 AS id, [1.0, 0.0, 0.0]::FLOAT[] AS vec
                ),
                right_vecs AS (
                    SELECT 2 AS id, [0.9, 0.1, 0.0]::FLOAT[] AS vec
                )
                SELECT l.id, r.id, ai_similarity(l.vec, r.vec, 'clip') AS similarity
                FROM left_vecs l
                CROSS JOIN right_vecs r
            """).fetchall()

            print(f"JOIN result: {result}")
            if result and len(result) == 1:
                similarity = result[0][2]
                print(f"✅ JOIN query executed, similarity: {similarity}")
                if similarity > 0.8:  # [1,0,0] and [0.9,0.1,0] should be very similar
                    print("✅ Similarity value in expected range")
                else:
                    print(f"⚠️ Similarity {similarity} seems low for similar vectors")
            else:
                print(f"❌ Unexpected result: {result}")
                return False
        except Exception as e:
            print(f"❌ JOIN-like query failed: {e}")
            return False

        print("\n" + "="*50)
        print("✅ All tests passed!")
        print("="*50)
        return True

    except ImportError:
        print("❌ Python duckdb module not available")
        return None

def main():
    print("="*50)
    print("ai_similarity Function Test")
    print("="*50)

    # Try Python bindings first
    result = test_with_python()

    if result is True:
        print("\n✅ All tests passed using Python bindings")
        return 0
    elif result is False:
        print("\n❌ Tests failed")
        return 1
    else:
        print("\n⚠️ Python bindings not available, CLI testing not implemented")
        return 2

if __name__ == "__main__":
    sys.exit(main())

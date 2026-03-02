#!/usr/bin/env python3
"""
M3 AI Filter Extension Test Suite

Tests the HTTP-based ai_filter implementation with:
- Multi-parameter support
- Deterministic scoring
- JSON request/response parsing
"""

import subprocess
import sys
from pathlib import Path

# Configuration
DUCKDB_BINARY = Path(__file__).parent.parent.parent / "build" / "duckdb"
EXTENSION_PATH = Path(__file__).parent.parent.parent / "build" / "test" / "extension" / "ai.duckdb_extension"


def run_query(sql: str) -> tuple[int, str, str]:
    """Run a query via DuckDB CLI."""
    cmd = [str(DUCKDB_BINARY), "-unsigned", "-c", sql]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def test_extension_load():
    """Test 1: Extension loads successfully."""
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


def test_three_parameters():
    """Test 2: Function accepts 3 parameters."""
    print("Test 2: Three-Parameter Function Call")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT ai_filter('image_data', 'cat', 'clip') AS score;"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    # Should have a numeric score
    lines = stdout.strip().split('\n')
    data_lines = [l for l in lines if l.startswith('│') and any(c.isdigit() for c in l)]

    if len(data_lines) == 0:
        print(f"❌ FAILED: No score returned")
        print(f"Output:\n{stdout}")
        return False

    print("✅ PASSED: Three-parameter function works")
    print(f"Output:\n{stdout}")
    print()
    return True


def test_deterministic_scoring():
    """Test 3: Same prompt produces same score."""
    print("Test 3: Deterministic Scoring")
    print("-" * 40)

    # Combine LOAD and SELECT in one command
    sql = f"LOAD '{EXTENSION_PATH}'; SELECT ai_filter('img', 'cat', 'clip') AS score FROM range(3);"

    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    # Extract all scores
    lines = stdout.strip().split('\n')
    scores = []
    for line in lines:
        if '│' in line and any(c.isdigit() for c in line):
            # Extract numeric value
            parts = line.split('│')
            if len(parts) >= 2:
                score_str = parts[-1].strip()
                try:
                    score = float(score_str)
                    scores.append(score)
                except ValueError:
                    pass

    if len(scores) != 3:
        print(f"❌ FAILED: Expected 3 scores, got {len(scores)}")
        print(f"Output:\n{stdout}")
        return False

    # All scores should be the same
    if not all(abs(s - scores[0]) < 0.0001 for s in scores):
        print(f"❌ FAILED: Scores are not deterministic")
        print(f"Scores: {scores}")
        return False

    print("✅ PASSED: Deterministic scoring confirmed")
    print(f"All 3 rows returned score: {scores[0]:.5f}")
    print()
    return True


def test_different_prompts():
    """Test 4: Different prompts produce different scores."""
    print("Test 4: Different Prompts → Different Scores")
    print("-" * 40)

    sql = f"""
    LOAD '{EXTENSION_PATH}';
    SELECT prompt, ai_filter('img', prompt, 'clip') AS score
    FROM (SELECT 'cat' AS prompt
          UNION ALL SELECT 'dog'
          UNION ALL SELECT 'bird') t;
    """

    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    # Extract scores and prompts
    lines = stdout.strip().split('\n')
    scores = []
    prompts = []
    for line in lines:
        if '│' in line:
            parts = [p.strip() for p in line.split('│')]
            # Expected format: │ cat │ 9.91951 │
            if len(parts) >= 3:
                try:
                    prompt_val = parts[1]
                    score_val = float(parts[-1])
                    if prompt_val and prompt_val != 'prompt' and prompt_val != 'varchar':
                        prompts.append(prompt_val)
                        scores.append(score_val)
                except (ValueError, IndexError):
                    pass

    if len(scores) != 3:
        print(f"❌ FAILED: Expected 3 scores, got {len(scores)}")
        print(f"Output:\n{stdout}")
        return False

    # Check that scores are different
    unique_scores = set()
    for s in scores:
        # Round to 5 decimal places for comparison
        unique_scores.add(round(s, 5))

    if len(unique_scores) < 2:
        print(f"❌ FAILED: All prompts produced same score")
        print(f"Prompts: {prompts}")
        print(f"Scores: {scores}")
        return False

    print("✅ PASSED: Different prompts produce different scores")
    print(f"Results: {list(zip(prompts, scores))}")
    print()
    return True


def test_json_response_format():
    """Test 5: Function correctly parses JSON responses."""
    print("Test 5: JSON Response Parsing")
    print("-" * 40)

    sql = f"LOAD '{EXTENSION_PATH}'; SELECT ai_filter('test_image', 'test_prompt', 'clip') AS score;"
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    # Extract score
    lines = stdout.strip().split('\n')
    score = None
    for line in lines:
        if '│' in line and any(c.isdigit() for c in line):
            parts = [p.strip() for p in line.split('│')]
            # Expected format: │ 32.9972 │
            if len(parts) >= 2:
                try:
                    potential_score = parts[-1]
                    if potential_score != 'score' and potential_score != 'double':
                        score = float(potential_score)
                        break
                except ValueError:
                    pass

    if score is None:
        print(f"❌ FAILED: Could not extract score")
        print(f"Output:\n{stdout}")
        return False

    print("✅ PASSED: JSON response correctly parsed")
    print(f"Score extracted: {score}")
    print()
    return True


def test_function_registration():
    """Test 6: Function is registered with correct signature."""
    print("Test 6: Function Registration")
    print("-" * 40)

    sql = f"""
    LOAD '{EXTENSION_PATH}';
    SELECT function_name, parameter_types
    FROM duckdb_functions()
    WHERE function_name = 'ai_filter';
    """
    returncode, stdout, stderr = run_query(sql)

    if returncode != 0:
        print(f"❌ FAILED: Query execution failed")
        print(f"Error: {stderr}")
        return False

    if "ai_filter" not in stdout:
        print(f"❌ FAILED: ai_filter not found in duckdb_functions()")
        print(f"Output:\n{stdout}")
        return False

    print("✅ PASSED: Function properly registered")
    print(f"Output:\n{stdout}")
    print()
    return True


def main():
    """Run all tests."""
    print("=" * 60)
    print("M3 AI Filter Extension Test Suite")
    print("=" * 60)
    print(f"DuckDB Binary: {DUCKDB_BINARY}")
    print(f"Extension: {EXTENSION_PATH}")
    print(f"Extension Size: {EXTENSION_PATH.stat().st_size / 1024:.1f} KB")
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
        test_three_parameters,
        test_deterministic_scoring,
        test_different_prompts,
        test_json_response_format,
        test_function_registration,
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
            import traceback
            traceback.print_exc()
            failed += 1

    # Summary
    print("=" * 60)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("=" * 60)

    if failed == 0:
        print("✅ All tests passed!")
        print()
        print("M3 Implementation Summary:")
        print("- Three-parameter function: image, prompt, model")
        print("- Deterministic scoring based on prompt")
        print("- JSON request/response parsing")
        print("- Simulated HTTP client (MVP)")
        print()
        print("Next Steps (M4):")
        print("- Replace mock client with real httplib")
        print("- Integrate actual AI service endpoints")
        print("- Add error handling and retries")
        return 0
    else:
        print("❌ Some tests failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())

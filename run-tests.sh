#!/bin/bash

# Configuration
TESTS_DIR="hw3-tests-staff"
RESULTS_DIR="results"
EXECUTABLE="./hw3"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE not found!"
    echo "Please run 'make' first to build the project."
    exit 1
fi

# Check if tests directory exists
if [ ! -d "$TESTS_DIR" ]; then
    echo "Error: Tests directory '$TESTS_DIR' not found!"
    exit 1
fi

# Create results directory
rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"

# Count number of tests
numtests=$(ls -1 "$TESTS_DIR"/t*.in 2>/dev/null | wc -l)

if [ $numtests -eq 0 ]; then
    echo "No test files found in $TESTS_DIR!"
    exit 1
fi

echo "Found $numtests tests in $TESTS_DIR"
echo "Running tests..."
echo ""

passed=0
failed=0

# Run all tests
for test_file in "$TESTS_DIR"/t*.in; do
    # Extract test number (e.g., t1.in -> 1)
    test_name=$(basename "$test_file" .in)
    test_num=${test_name#t}
    
    expected_file="$TESTS_DIR/t$test_num.out"
    result_file="$RESULTS_DIR/t$test_num.res"
    
    # Check if expected output exists
    if [ ! -f "$expected_file" ]; then
        echo "⚠️  Test $test_num: Missing expected output file"
        continue
    fi
    
    # Run the test
    $EXECUTABLE < "$test_file" &> "$result_file"
    
    # Compare results
    if diff -q "$result_file" "$expected_file" &> /dev/null; then
        echo "✅ Test $test_num: PASSED"
        ((passed++))
    else
        echo "❌ Test $test_num: FAILED"
        ((failed++))
        echo "   Expected output saved in: $expected_file"
        echo "   Your output saved in: $result_file"
        echo "   Run: diff $result_file $expected_file"
        echo ""
    fi
done

echo ""
echo "======================================"
echo "Results: $passed passed, $failed failed out of $((passed + failed)) tests"
echo "All results saved in: $RESULTS_DIR/"
echo "======================================"

if [ $failed -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "❌ Some tests failed. Check the diffs above."
    exit 1
fi

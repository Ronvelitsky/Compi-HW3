#!/bin/bash

# Directories
program="./hw3"
input_dir="tests"

for input_file in "$input_dir"/*.in; do
    test_name=$(basename "$input_file" .in)
    
    expected_file="$input_dir/$test_name.out"
    output_file="$input_dir/$test_name.res"
    
    "$program" < "$input_file" > "$output_file"
    
    if [[ -f "$expected_file" ]]; then
        if diff -q "$expected_file" "$output_file" > /dev/null; then
            echo -e "    \033[1;32mPASS\033[0m - $test_name"
        else
            echo
            echo -e "    \033[1;31mFAIL\033[0m - $test_name"
            echo -e "\033[1;31mdiff $expected_file $output_file\033[0m"
            diff --color "$expected_file" "$output_file"
            echo ---
            echo -e "\033[1;4m$input_file\033[0m:"
            cat "$input_file"
            echo
            echo ---
            echo
        fi
    else
        echo -e "${RED}Expected file not found for test: $test_name${NC}"
    fi
done

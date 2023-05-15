#!/bin/bash

output_file="file.c"
expected_output_file="expected_output.txt"

output=$(cat "$output_file")
expected_output=$(cat "$expected_output_file")

if [ "$output" = "$expected_output" ]
then
  echo "Success! The output file matches the expected output file."
else
  echo "Error: The output file does not match the expected output file."
  echo "Expected output:"
  echo "$expected_output"
  echo "Actual output:"
  echo "$output"
fi


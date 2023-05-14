def compare_files(output_file, expected_output_file):
    with open(output_file, "r") as f:
        output = f.read()
    with open(expected_output_file, "r") as f:
        expected_output = f.read()
    if output == expected_output:
        print("Success! The output file matches the expected output file.")
    else:
        print("Error: The output file does not match the expected output file.")
        print("Expected output:")
        print(expected_output)
        print("Actual output:")
        print(output)

# Example usage
output_file = input("Enter Output file: ")
expected_output_file = input("Enter expected file: ")

compare_files(output_file, expected_output_file)

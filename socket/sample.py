# sample_script.py

import sys

def process_argument(argument):
    # Process the argument in some way
    result = f"Processed argument: {argument}"
    return result

if __name__ == "__main__":
    # Check if the command-line argument is provided
    if len(sys.argv) < 2:
        print("Usage: python sample_script.py <argument>")
        sys.exit(1)

    # Get the argument from the command line
    argument = sys.argv[1]

    # Process the argument
    processed_argument = process_argument(argument)

    # Print the processed argument
    print(processed_argument)


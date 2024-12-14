#!/usr/bin/env python

import re
import sys

def transform_code(code):
    # Regex pattern for transformation
    pattern = r'(\w+\s*\*)\s*(\w+)\s*=\s*malloc\((.*)\);'

    # Perform the transformation using regex and substitution
    transformed_code = re.sub(pattern, r'\1 \2 = static_cast<\1>(malloc(\3));', code)
    return transformed_code

def main():
    try:
        # Read code from standard input
        code = sys.stdin.read()

        # Perform the transformation
        transformed_code = transform_code(code)

        # Write transformed code to standard output
        sys.stdout.write(transformed_code)
    except Exception as e:
        print("Error occurred during transformation:", str(e))

if __name__ == "__main__":
    main()


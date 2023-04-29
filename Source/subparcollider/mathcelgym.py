import math
import random

def generate_addition_question(max_digits: int) -> str:
    min_value = 1
    max_value = (10 ** max_digits) - 1

    operand1 = random.randint(min_value, max_value)
    operand2 = random.randint(min_value, max_value)

    return f"{operand1} + {operand2}"


def evaluate_expression(expression: str) -> float:
    try:
        # Whitelist functions and variables for the AI
        allowed_functions = {
            "sin": math.sin,
            "cos": math.cos,
            "tan": math.tan,
            "sqrt": math.sqrt,
            "pi": math.pi,
        }

        result = eval(expression, {"__builtins__": None}, allowed_functions)
        return result
    except Exception as e:
        print(f"Error: {e}")
        return None

def main():
    while True:
        max_digits = int(input("Enter the maximum number of digits (q to quit): "))
        if max_digits < 1:
            print("Please enter a positive integer.")
            continue

        expression = generate_addition_question(max_digits)
        print(f"Generated addition question: {expression}")

        result = evaluate_expression(expression)
        if result is not None:
            print(f"Result: {result}")

if __name__ == "__main__":
    main()


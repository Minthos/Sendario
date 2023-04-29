import math
import random

import torch
import torch.nn as nn
import torch.optim as optim


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

# Generate the dataset
def generate_dataset(num_samples, max_digits):
    data = []
    for _ in range(num_samples):
        expression = generate_addition_question(max_digits)
        result = evaluate_expression(expression)
        data.append((expression, result))
    return data

# Define the neural network
class SimpleNN(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super(SimpleNN, self).__init__()
        self.layer1 = nn.Linear(input_size, hidden_size)
        self.layer2 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        x = torch.relu(self.layer1(x))
        x = self.layer2(x)
        return x

# Prepare the data
def prepare_data(data, max_digits):
    X = []
    Y = []

    for expression, result in data:
        # Convert expression to feature vector
        tokens = expression.split()
        x = [int(tokens[0]), int(tokens[2])]
        X.append(x)
        Y.append(result)

    X = torch.tensor(X, dtype=torch.float32)
    Y = torch.tensor(Y, dtype=torch.float32).view(-1, 1)

    return X, Y

# Train the neural network
def train(model, criterion, optimizer, X, Y, epochs):
    for epoch in range(1, epochs + 1):
        optimizer.zero_grad()

        outputs = model(X)
        loss = criterion(outputs, Y)

        loss.backward()
        optimizer.step()

        if epoch % 10 == 0:
            print(f"Epoch: {epoch}, Loss: {loss.item()}")

# Main function
def main():
    # Generate the dataset
    num_samples = 5000
    max_digits = 3
    data = generate_dataset(num_samples, max_digits)

    # Prepare the data
    X, Y = prepare_data(data, max_digits)

    # Define the neural network
    input_size = 2
    hidden_size = 64
    output_size = 1
    model = SimpleNN(input_size, hidden_size, output_size)

    # Define the loss function and optimizer
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)

    # Train the neural network
    epochs = 100
    train(model, criterion, optimizer, X, Y, epochs)

    # Test the trained model
    test_expression = generate_addition_question(max_digits)
    print(f"Test expression: {test_expression}")

    test_x, _ = prepare_data([(test_expression, 0)], max_digits)
    test_output = model(test_x).item()

    print(f"Predicted result: {test_output}")
    print(f"Actual result: {evaluate_expression(test_expression)}")

if __name__ == "__main__":
    main()


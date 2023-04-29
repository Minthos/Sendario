import math
import random

import torch
import torch.nn as nn
import torch.optim as optim

num_samples = 50000
difficulty = 1

# 
allowed_functions = {
    "sin": math.sin,
    "cos": math.cos,
    "tan": math.tan,
    "sqrt": math.sqrt,
    "pi": math.pi,
}

def generate_question(difficulty: int) -> str:
    min_value = 1
    max_value = (10 ** difficulty) - 1

    operand1 = random.randint(min_value, max_value)
    operand2 = random.randint(min_value, max_value)

    return f"{operand1} + {operand2}"


def evaluate_expression(expression: str) -> float:
    try:
        result = eval(expression, {"__builtins__": None}, allowed_functions)
        return result
    except Exception as e:
        print(f"Error: {e}")
        return None

def generate_dataset(num_samples, difficulty):
    data = []
    for _ in range(num_samples):
        expression = generate_question(difficulty)
        result = evaluate_expression(expression)
        data.append((expression, result))
    return data

class SimpleNN(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super(SimpleNN, self).__init__()
        self.layer1 = nn.Linear(input_size, hidden_size)
        self.layer2 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        x = torch.relu(self.layer1(x))
        x = self.layer2(x)
        return x

def prepare_data(data, difficulty):
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

def test_and_print(difficulty, model):
    # Test the trained model
    test_expression = generate_question(difficulty)
    test_x, _ = prepare_data([(test_expression, 0)], difficulty)
    test_output = model(test_x).item()
    print(f"{test_expression} = {test_output} ({evaluate_expression(test_expression)})")


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
            test_and_print(difficulty, model)

def main():
    data = generate_dataset(num_samples, difficulty)
    X, Y = prepare_data(data, difficulty)
    input_size = 2
    hidden_size = 64
    output_size = 1
    model = SimpleNN(input_size, hidden_size, output_size)
    # Define the loss function and optimizer
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    epochs = 100
    train(model, criterion, optimizer, X, Y, epochs)
    test_and_print(difficulty, model)

if __name__ == "__main__":
    main()


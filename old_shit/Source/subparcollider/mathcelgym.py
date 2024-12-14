import math
import random

import torch
import torch.nn as nn
import torch.optim as optim

difficulty = 1
epochs = 1000000000
samples_per_difficulty = 10000
num_samples = 10000
input_size = 20
hidden_size = 128
output_size = 1

# Operators, allowed functions, the numbers 0 to 9 go into the vocabulary.
operators = list("+-*/")
other_characters = list("() ")
# These are the functions that eval will accept from the AI. They go into the question generator's vocabulary.
# Higher difficulty levels unlock more functions.
allowed_functions = {
    "sin": math.sin,
    "cos": math.cos,
    "tan": math.tan,
    "sqrt": math.sqrt,
    "pi": math.pi,
}

def generate_question(difficulty: int) -> str:
    min_value = 0
    max_value = (3 ** difficulty)

    operand1 = random.randint(min_value, max_value)
    operand2 = random.randint(min_value, max_value)

    operator = operators[random.randint(0, len(operators)-1)]
    if operator == "/" and operand2 == 0:
        operand2 = 1
    return f"{operand1} {operator} {operand2}"


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
        self.layer2 = nn.Linear(hidden_size, hidden_size)
        self.layer3 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        x = torch.relu(self.layer1(x))
        x = self.layer2(x)
        x = self.layer3(x)
        return x

def tokenize(expression):
    tokens = []
    for i in range(len(expression)):
        if expression[i].isdigit():
            tokens.append(int(expression[i]))
        else:
            tokens.append((operators + other_characters).index(expression[i]) + 11)
    if len(tokens) < input_size:
        tokens += [10] * (input_size - len(tokens))
    return tokens

def prepare_data(data, difficulty):
    X = []
    Y = []

    for expression, result in data:
        # Convert expression to feature vector
        tokens = tokenize(expression)
        X.append(tokens)
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
    global difficulty, data, num_samples_per_difficulty, num_samples
    for epoch in range(1, epochs + 1):
        optimizer.zero_grad()
        outputs = model(X)
        loss = criterion(outputs, Y)
        loss.backward()
        optimizer.step()
        if epoch % 500 == 0:
            print(f"Epoch: {epoch}, Loss: {loss.item()}")
            test_and_print(difficulty, model)
            if loss.item() < 0.1:
                difficulty += 1
                data += generate_dataset(samples_per_difficulty, difficulty)
                X, Y = prepare_data(data, difficulty)
                print(f"Difficulty increased to {difficulty}")
                print(f"New dataset size: {len(data)}")
                num_samples = difficulty * samples_per_difficulty

data = generate_dataset(num_samples, difficulty)
X, Y = prepare_data(data, difficulty)

def main():
    model = SimpleNN(input_size, hidden_size, output_size)
    # Define the loss function and optimizer
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    train(model, criterion, optimizer, X, Y, epochs)
    test_and_print(difficulty, model)

if __name__ == "__main__":
    main()


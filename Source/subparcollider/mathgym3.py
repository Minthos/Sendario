import math
import random

import torch
import torch.nn as nn
import torch.optim as optim
from torch.nn.utils.rnn import pad_sequence

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

class TransformerModel(nn.Module):
    def __init__(self, vocab_size, d_model, nhead, dim_feedforward, num_layers):
        super(TransformerModel, self).__init__()
        self.embedding = nn.Embedding(vocab_size, d_model)
        self.transformer = nn.Transformer(d_model=d_model, nhead=nhead, num_encoder_layers=num_layers, num_decoder_layers=num_layers, dim_feedforward=dim_feedforward)
        self.decoder = nn.Linear(d_model, 1)

    def forward(self, x):
        x = self.embedding(x)
        x = x.permute(1, 0, 2)
        output = self.transformer(x, x)
        output = self.decoder(output[-1, :, :])
        return output

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

    X = pad_sequence([torch.LongTensor(x) for x in X], batch_first=True)
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
        if epoch % 5 == 0:
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
    model = TransformerModel(vocab_size=32, d_model=128, nhead=4, dim_feedforward=512, num_layers=2)
    # Define the loss function and optimizer
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    train(model, criterion, optimizer, X, Y, epochs)
    test_and_print(difficulty, model)

if __name__ == "__main__":
    main()


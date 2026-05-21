import random
import argparse

parser = argparse.ArgumentParser(description='Generate a sample file with random card sequences.')
parser.add_argument('num_entries', type=int, help='Number of random card sequences to generate')
parser.add_argument('-o','--output-file', type=str, help='Output file name for the generated sample')
args = parser.parse_args()

num_entries = args.num_entries
output_file = args.output_file

deck = []
for num in range(1,14):
    for _ in range(4):
        deck.append(num)


with open(output_file, 'w') as f:
    f.write(f'{num_entries}\n')
    for _ in range(num_entries):
        random.shuffle(deck)
        f.write(' '.join(map(str, deck)) + '\n')
import random
import argparse
import time
import multiprocessing


def generate_sequences(num_entries, seed=None):
    out_list = []
    deck = []
    for num in range(1,14):
        for _ in range(4):
            deck.append(num)
    local_rng = random.Random(seed)  
    for _ in range(num_entries):
        local_rng.shuffle(deck)
        out_list.append(' '.join(map(str, deck)) + '\n')
    return out_list

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a sample file with random card sequences.')
    parser.add_argument('num_entries', type=int, help='Number of random card sequences to generate')
    parser.add_argument('-o','--output-file', type=str, help='Output file name for the generated sample')
    parser.add_argument('-n', '--num-threads', type=int, help='Number of threads to use for generation', default=1)
    args = parser.parse_args()

    num_entries = args.num_entries
    output_file = args.output_file
    num_threads = args.num_threads

    base_seed = time.time()
    tasks = [(num_entries// num_threads, base_seed + i) for i in range(num_threads)]
    with multiprocessing.Pool(processes=num_threads) as pool:
        results = pool.starmap(generate_sequences, tasks)

    with open(output_file, 'w') as f:
        f.write(f'{num_entries}\n')
        for result in results:
            f.writelines(result)
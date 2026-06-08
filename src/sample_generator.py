import random
import argparse
import time
import multiprocessing
from bitarray import bitarray

def generate_sequences(num_entries, seed=None):
    out_bit_array = bitarray(endian="big")
    deck = []
    for num in range(1,14):
        for _ in range(4):
            deck.append(format(num, "04b"))
    local_rng = random.Random(seed)  
    for _ in range(num_entries):
        local_rng.shuffle(deck)
        b = bitarray("".join(deck))
        out_bit_array += b
    return out_bit_array

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a sample file with random card sequences.')
    parser.add_argument('num_entries', type=int, help='Number of random card sequences to generate')
    parser.add_argument('-o','--output-file', type=str, help='Output file name for the generated sample')
    #parser.add_argument('-n', '--num-threads', type=int, help='Number of threads to use for generation', default=1)
    #parser.add_argument('-c', '--chunk-count', type=int, help='Number of chunks to generate the sample (only a portion of the sample will be kept in memory using number of entries / chunk_count) (default: 1)', default=1)
    args = parser.parse_args()

    num_entries = int(args.num_entries)
    output_file = args.output_file
    #num_threads = args.num_threads
    #chunk_count = args.chunk_count

    base_seed = time.time()
    bit_arr = generate_sequences(num_entries, base_seed)
    with open("output.bin", "wb") as f:
        f.write(bit_arr)
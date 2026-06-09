import random
import argparse
import time
import multiprocessing
from bitarray import bitarray

def generate_sequences(num_entries, seed, output_file, file_lock, num_of_writes):
    deck = []
    for num in range(1,14):
        for _ in range(4):
            deck.append(format(num, "04b"))
    local_rng = random.Random(seed)  

    entries_per_write = num_entries // num_of_writes
    remaining_entries = num_entries % num_of_writes
    for i in range(num_of_writes):
        current_entries = entries_per_write + (1 if i < remaining_entries else 0)

        out_bit_array = bitarray(endian="big")
        for _ in range(current_entries):
            local_rng.shuffle(deck)
            b = bitarray("".join(deck), endian="big")
            out_bit_array += b

        with file_lock:
            with open(output_file, 'ab') as f:
                out_bit_array.tofile(f)
        out_bit_array.clear()

if __name__ == "__main__":
    multiprocessing.set_start_method('fork')

    parser = argparse.ArgumentParser(description='Generate a sample file with random card sequences.')
    parser.add_argument('num_entries', type=int, help='Number of random card sequences to generate')
    parser.add_argument('-o','--output-file', type=str, help='Output file name for the generated sample')
    parser.add_argument('-n', '--num-threads', type=int, help='Number of threads to use for generation', default=1)
    parser.add_argument('-m', '--max-memory', type=int, help='Maximum memory to use for generation in MB', default=512)
    args = parser.parse_args()

    num_entries = int(args.num_entries)
    output_file = args.output_file
    num_threads = int(args.num_threads)
    max_memory = int(args.max_memory)

    base_seed = time.time()
    processes = []

    file_lock = multiprocessing.Lock()

    bytes_per_entry = 26
    total_bytes = num_entries * bytes_per_entry
    max_memory_bytes = max_memory * 1024 * 1024

    num_of_writes = 1

    if total_bytes > max_memory_bytes:
        num_of_writes = (total_bytes // max_memory_bytes) + 1

    entries_per_write = num_entries // num_threads
    remaining_entries = num_entries % num_threads

    open(output_file, 'wb').close()

    for i in range(num_threads):
        process_entries = entries_per_write + (1 if i < remaining_entries else 0)
        p = multiprocessing.Process(target=generate_sequences, args=(process_entries, base_seed + i, output_file, file_lock, num_of_writes))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()

    print(f"Generated {num_entries} random card sequences in {output_file} using {num_threads} threads with a maximum memory limit of {max_memory} MB.")
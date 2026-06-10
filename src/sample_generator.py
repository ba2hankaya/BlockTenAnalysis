import numpy as np
import argparse
import time
import threading

def generate_sequences(num_entries, seed, output_file, file_lock, num_of_writes):
    deck = np.repeat(np.arange(1,14, dtype=np.uint8), 4)
    local_rng = np.random.default_rng(seed = seed)

    entries_per_write = num_entries // num_of_writes
    remaining_entries = num_entries % num_of_writes
    for i in range(num_of_writes):
        current_entries = entries_per_write + (1 if i < remaining_entries else 0)

        decks = np.tile(deck, (current_entries, 1))
        local_rng.permuted(decks, axis=1, out=decks)

        packed_bytes = (decks[:, 0::2] << 4) | (decks[:, 1::2] & 0x0F)

        del decks

        raw_bytes = packed_bytes.tobytes()

        with file_lock:
            with open(output_file, 'ab') as f:
                f.write(raw_bytes)
        
        del raw_bytes

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a sample file with random card sequences.')
    parser.add_argument('num_entries', type=int, help='Number of random card sequences to generate')
    parser.add_argument('-o','--output-file', type=str, help='Output file name for the generated sample')
    parser.add_argument('-n', '--num-threads', type=int, help='Number of threads to use for generation', default=1)
    parser.add_argument('-m', '--max-memory', type=int, help='Maximum memory to use for generation in MB', default=512)
    parser.add_argument('--append', action='store_true', help='Append to the output file instead of overwriting it')
    args = parser.parse_args()

    num_entries = int(args.num_entries)
    output_file = args.output_file
    num_threads = int(args.num_threads)
    max_memory = int(args.max_memory)
    append = args.append
    main(num_entries, output_file, num_threads, max_memory, append=append)

def main(num_entries, output_file, num_threads, max_memory, append=False):
    base_seed = int(time.time()*1000)
    threads = []

    file_lock = threading.Lock()

    bytes_per_entry = 26
    deck_bytes_per_entry = 52 * np.dtype(np.uint8).itemsize
    peak_bytes_per_entry = bytes_per_entry + deck_bytes_per_entry

    max_memory_bytes = max_memory * 1024 * 1024

    entries_per_thread = num_entries // num_threads
    remaining_entries = num_entries % num_threads

    per_thread_max_memory = max_memory_bytes // num_threads

    if not append:
        open(output_file, 'wb').close()

    for i in range(num_threads):
        thread_entries = entries_per_thread + (1 if i < remaining_entries else 0)

        num_of_writes_thread = 1
        total_thread_peak = thread_entries * peak_bytes_per_entry
        if total_thread_peak > per_thread_max_memory:
            num_of_writes_thread = (total_thread_peak // per_thread_max_memory) + 1

        t = threading.Thread(
            target=generate_sequences,
            args=(thread_entries, base_seed + i, output_file, file_lock, num_of_writes_thread)
        )

        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    append_text = "Appended " if append else "Generated "
    print(append_text + f"{num_entries} random card sequences in {output_file} using {num_threads} threads with a maximum memory limit of {max_memory} MB.")
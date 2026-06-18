# blockTenAnalysis

## Overview

`blockTenAnalysis` is a performance-focused project for analyzing compressed card sequences and benchmarking a multithreaded simulator. It includes:

- a C++ simulator with a thread-safe queue for buffered file processing
- a sample generator for binary card-sequence datasets
- a benchmarking and plotting pipeline for performance experiments

The project operates on binary sample files where each 26-byte entry encodes a 52-card sequence. The simulator evaluates each sequence using a pairing score model and reports aggregated results.

## Repository structure

- `build/`
  - `simulator` - compiled executable produced from `src/simulator.cpp` and `src/concurrentque.cpp`
- `plots/`
  - generated visualizations produced by `src/perf_analyzer.py`
- `results/`
  - benchmark JSON output files created by `src/perf_analyzer.py`
- `samples/`
  - generated `.bin` sample files used as simulator inputs
- `src/`
  - `concurrentque.hpp` - thread-safe queue class definition
  - `concurrentque.cpp` - queue implementation
  - `simulator.cpp` - main simulator and processing pipeline
  - `sample_generator.py` - binary sample generation utility
  - `perf_analyzer.py` - benchmarking and plotting controller
- `game_analyzer.py` - analyzes simulator result JSON files and generates score distribution plots

### `src/concurrentque.hpp`

Defines the `ConcurrentQueue` class used by the simulator to safely pass buffer payloads between producer and worker threads.

Key fields:
- `Node` - linked-list node containing `char*` buffer data and length
- `head`, `tail` - queue pointers
- `mtx` - mutex for exclusive access
- `cv_not_full`, `cv_not_empty` - condition variables for producer/consumer coordination
- `cur_size`, `max_size` - current and maximum queue memory usage
- `is_shutdown` - indicator to stop workers cleanly

Public methods:
- `ConcurrentQueue(int max_size)` - constructs a queue with a memory limit in bytes
- `void Enqueue(char* p_buffer, int size)` - waits until space is available, then appends a buffer
- `int Dequeue(char*& p_buffer, int& size)` - waits for an item or shutdown; returns `-1` when queue is closed
- `void Shutdown()` - signals shutdown and wakes waiting threads

### `src/concurrentque.cpp`

Implements the thread-safe queue behavior:
- `Enqueue()` uses `cv_not_full.wait(...)` to enforce the `max_size` limit
- `Dequeue()` uses `cv_not_empty.wait(...)` and returns a buffer for worker processing
- `Shutdown()` awakens all waiting threads so they can exit gracefully

### `src/simulator.cpp`

The simulator reads binary sample files and processes them in parallel.

Main responsibilities:
- parse command-line arguments:
  - `<input_file>` - required binary sample file
  - `-o <output_file>` - optional JSON output path
  - `-m <max_memory_usage_in_MB>` - optional memory cap
  - `-w <num_workers>` - optional number of worker threads
- configure buffer sizes and queue capacity based on memory limits
- launch worker threads that process buffered chunks from the queue
- aggregate per-thread results and write JSON output

Important functions:
- `score_of_subarray(char* buffer, int start_index)`
  - evaluates a 26-byte encoded card sequence
  - uses card pairing rules and active-card limits to compute a score
  - returns the number of completed pairs before the game ends or the sequence ends
- `process_buffer(char* buffer, int* result_array, std::streamsize bytes_read)`
  - scans a loaded buffer in 26-byte increments and counts scores
- `producer(std::ifstream& file, int buffer_size, ConcurrentQueue& queue, int* file_result)`
  - reads the input file in chunks, enqueues each buffer, and shuts down the queue when finished
- `worker(int id, int* results, ConcurrentQueue& queue)`
  - dequeues buffers, processes them, and releases memory

Memory handling:
- the simulator keeps `buffer_size` aligned to 26 bytes so every read contains whole entries
- `max_memory_bytes` is converted from MB and trimmed to a multiple of 26 bytes if needed
- the queue is sized using a quarter of the requested max memory, leaving room for worker-local arrays and overhead

Output format:
- JSON containing `time`, `games`, `memory_bytes`, plus per-score counts from `0` to `24`

### `src/sample_generator.py`

Generates random card-sequence sample data for the simulator.

Key behavior:
- each entry represents a 52-card deck encoded into 26 bytes:
  - cards are stored as 4-bit values
  - two cards per byte
- uses NumPy to build repeated decks of values `1..13` and randomly permutes each deck
- writes raw bytes directly to a binary output file
- can run with multiple threads and a memory limit to split writes across several passes

Public API:
- `main(num_entries, output_file, num_threads, max_memory, append=False)`
  - creates `num_entries` random sequences in `output_file`
  - `num_threads` enables concurrent generation
  - `max_memory` bounds memory usage in MB
  - `append=True` will append to an existing output file instead of overwriting

Usage example:
- `python3 sample_generator.py 100000 -o ../samples/example.bin -n 4 -m 512`

### `src/perf_analyzer.py`

A command-line toolkit for generating samples, running simulator benchmarks, and producing plots.

Commands:
- `generate`
  - creates binary sample files from a range of powers of 10
  - uses `sample_generator.main(...)`
- `simulate`
  - runs the compiled simulator across sample files in `../samples/`
  - sweeps thread counts and memory budgets
  - collects timing results and writes `../results/performance_results.json`
- `plot`
  - reads JSON results and generates 3D plots in `../plots`
  - supports scatter or surface renderings

Benchmark output keys:
- `bin_file` - sample file path
- `threads` - thread count used
- `max_memory_mb` - memory budget used
- `mean_time`, `std_time` - aggregated timing statistics
- `ci_95_low`, `ci_95_high` - 95% confidence interval
- `raw_times` - individual trial durations

Plot conventions:
- X axis: normalized input size
- Y axis: `log2(max_memory_mb)`
- Z axis: execution time

### `src/game_analyzer.py`

Analyzes JSON score distribution files produced by the simulator and generates publication-ready plots.

Key behavior:
- reads a `bin_results.json` file containing score counts for values `0` through `24`
- normalizes counts to percentage distribution values
- generates either a bar chart or a circle (pie) chart
- writes PNG output to the specified directory and can optionally show the chart interactively

Usage:
- `python3 game_analyzer.py -i ../results/1e4.00.bin_results.json -o ../plots -t bar`
- `python3 game_analyzer.py -i ../results/1e4.00.bin_results.json -o ../plots -t circle --show`

## Usage

### Build the simulator

From the project root:

```bash
cd src
mkdir -p ../build
g++ -std=c++17 -O2 simulator.cpp concurrentque.cpp -pthread -o ../build/simulator
```

### Generate sample files

From `src/`:

```bash
python3 perf_analyzer.py generate --min 4 --max 7 --num 4
```

Or directly with the generator:

```bash
python3 sample_generator.py 100000 -o ../samples/example.bin -n 4 -m 512
```

### Run the simulator benchmark

From `src/`:

```bash
python3 perf_analyzer.py simulate --threads 4 --memory 10 --attempts 5
```

This reads all `.bin` files in `../samples/`, runs the compiled simulator, and writes `../results/performance_results.json`.

### Plot results

From `src/`:

```bash
python3 perf_analyzer.py plot
```

For surface plots instead of scatter:

```bash
python3 perf_analyzer.py plot --surface
```

## Notes

- `perf_analyzer.py` assumes the simulator executable is available at `../build/simulator` when run from `src/`.
- `sample_generator.py` writes binary sample files in a compact 26-byte-per-entry format.
- `simulator.cpp` uses a producer/consumer pipeline and worker threads to maximize throughput while respecting a memory cap.

## Dependencies

- C++17 compiler with pthread support
- Python 3 with `numpy` and `matplotlib` for analyzer scripts
- Python 3
- NumPy
- SciPy
- Matplotlib

Install Python dependencies with:

```bash
python3 -m pip install numpy scipy matplotlib
```

## Project Goals

This repository is designed to:
- generate realistic binary datasets of card sequences
- measure simulator performance under varying thread and memory budgets
- analyze runtime behavior and visualize scaling effects
- support efficient processing of large sample files using a thread-safe queue

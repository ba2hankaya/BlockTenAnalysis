import scipy.stats as stats
import os
import subprocess
import numpy as np
import json
import argparse

import sample_generator

def generate_samples(min_power, max_power, num_samples):
    powers = np.linspace(min_power, max_power, num=num_samples)
    
    for power in powers:
        n = int(10 ** power)
        bin_name = f"../samples/1e{power:.2f}.bin"
        sample_generator.main(n, bin_name, 16, 512)

def run_simulator(pow_thread, pow_mem, attempts_per_config):
    directory = "../samples/"
    results = []
    
    for filename in os.listdir(directory):
        full_path = os.path.join(directory, filename)
        if os.path.isfile(full_path) and filename.endswith(".bin"):
            bin_name = full_path
            
            for t_idx in range(pow_thread + 1):
                threads = 2 ** t_idx
                for mm_idx in range(pow_mem + 1):
                    max_mem = 2 ** mm_idx
                    config_times = []
                    
                    for _ in range(attempts_per_config):
                        args = [bin_name, "-w", str(threads), "-m", str(max_mem)]
                        
                        try:
                            raw_output = subprocess.run(
                                ["../build/simulator"] + args, 
                                capture_output=True, text=True, check=True
                            ).stdout.strip()
                            
                            result_json = raw_output.split('\n')[-1] 
                            result = json.loads(result_json)

                            time_taken = result["time"]
                            n = result["games"]
                            mem_used_bytes = result["memory_bytes"]

                            config_times.append(time_taken)
                        except Exception as e:
                            print(f"Error running simulator: {e}")
                            continue

                    if not config_times:
                        continue

                    times = np.array(config_times)
                    mean = np.mean(times)
                    std = np.std(times, ddof=1)
                    times_count = len(times)

                    confidence = 0.95
                    t_crit = stats.t.ppf((1 + confidence) / 2, df=times_count - 1)
                    margin = t_crit * std / np.sqrt(times_count)
                    ci_low = mean - margin
                    ci_high = mean + margin

                    results.append({
                        "bin_file": bin_name,
                        "threads": threads,
                        "max_memory_mb": mem_used_bytes / (1024 * 1024),
                        "mean_time": mean,
                        "std_time": std,
                        "ci_95_low": ci_low,
                        "ci_95_high": ci_high,
                        "attempts_for_this_config": times_count,
                        "n" : n,
                        "raw_times": config_times
                    })


                    print(f"Completed sim {len(config_times)} times with args: {args}")
                    
    with open("../results/performance_results.json", 'w') as f:
        json.dump(results, f, indent=4)

def read_results():
    with open("../results/performance_results.json", 'r') as f:
        results = json.load(f)
    return results

def group_results_by_threads(results):
    grouped = {}
    for result in results:
        thread_num = result["threads"]
        if thread_num not in grouped:
            grouped[thread_num] = []
        grouped[thread_num].append(result)       
    return grouped

def plot_to_3d_graph(grouped_results, surface=False):
    import matplotlib
    matplotlib.use('Qt5Agg')

    import matplotlib.pyplot as plt

    output_dir = "../plots"
    os.makedirs(output_dir, exist_ok=True)

    for threads, data in grouped_results.items():
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        x = np.array([d["n"] for d in data]) / np.min(np.array([d["n"] for d in data])) # n
        y = np.array([np.log2(d["max_memory_mb"]) for d in data]) # max_memory in MB 
        z = np.array([d["mean_time"] for d in data])  # execution time

        if surface:
            ax.plot_trisurf(x, y, z, cmap='viridis', edgecolor='none', alpha=0.8)
        else:
            ax.scatter(x, y, z, label=f'Threads : {threads}')

        ax.set_xlabel(f'Input Size (n) *1e{np.log10(np.min(np.array([d["n"] for d in data])))}')
        ax.set_ylabel('Max Memory (log2)')
        ax.set_zlabel('Execution Time (s)')
        ax.set_title(f'Number of threads  = {threads}')
        ax.legend()
        
        file_suffix = "surface" if surface else "scatter"
        file_path = os.path.join(output_dir, f"threads_{threads}_{file_suffix}.png")
        plt.savefig(file_path, bbox_inches='tight', pad_inches=0.5, dpi=300)
        print(f"Saved plot: {file_path}")

    plt.show()

def main():
    parser = argparse.ArgumentParser(description="Simulator Benchmarking & Visualization Toolkit")
    subparsers = parser.add_subparsers(dest="command", required=True, help="Available workflows")

    # Command 1: Generate
    parser_gen = subparsers.add_parser("generate", help="Generate binary sample files")
    parser_gen.add_argument("--min", type=float, default=1.0, help="Min power of 10 for input size")
    parser_gen.add_argument("--max", type=float, default=5.0, help="Max power of 10 for input size")
    parser_gen.add_argument("--num", type=int, default=10, help="Total number of samples to generate")

    # Command 2: Simulate
    parser_sim = subparsers.add_parser("simulate", help="Run the simulator on the generated samples")
    parser_sim.add_argument("--threads", type=int, default=4, help="Max power of 2 for threads")
    parser_sim.add_argument("--memory", type=int, default=10, help="Max power of 2 for memory")
    parser_sim.add_argument("--attempts", type=int, default=5, help="Number of attempts per configuration for averaging")

    # Command 3: Plot 
    parser_plot = subparsers.add_parser("plot", help="Read JSON results and generate 3D plots")
    parser_plot.add_argument("--surface", action='store_true', help="Generate surface plots instead of scatter")

    args = parser.parse_args()

    if args.command == "generate":
        print(f"Generating {args.num} samples from 10^{args.min} to 10^{args.max}...")
        os.makedirs("../samples", exist_ok=True)
        generate_samples(args.min, args.max, args.num)
        print("Done.")

    elif args.command == "simulate":
        print(f"Running simulator (Threads limit: 2^{args.threads}, Memory limit: 2^{args.memory})...")
        run_simulator(args.threads, args.memory, args.attempts)
        print("Simulations complete. Data saved to ../results/performance_results.json.")

    elif args.command == "plot":
        print("Reading results and rendering plots...")
        results = read_results()
        grouped = group_results_by_threads(results)
        plot_to_3d_graph(grouped, surface=args.surface)

if __name__ == "__main__":
    main()
import sample_generator
import os
import subprocess
import numpy as np
import json
import argparse

def generate_samples(min_power, max_power, num_samples):
    powers = np.linspace(min_power, max_power, num=num_samples)
    
    for power in powers:
        n = int(10 ** power)
        bin_name = f"../samples/1e{power:.2f}.bin"
        sample_generator.main(n, bin_name, 16, 512)

def run_simulator(pow_thread, pow_mem):
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
                    intermediate_results = []
                    
                    for _ in range(10):
                        args = [bin_name, "-w", str(threads), "-m", str(max_mem)]
                        print(f"Running with args: {args}")
                        
                        try:
                            raw_output = subprocess.run(
                                ["../build/simulator"] + args, 
                                capture_output=True, text=True, check=True
                            ).stdout.strip()
                            
                            result_json = raw_output.split('\n')[-1] 
                            result = json.loads(result_json)
                            intermediate_results.append(result)
                        except Exception as e:
                            print(f"Error running simulator: {e}")
                            continue
                            
                    if not intermediate_results:
                        continue
                        
                    execution_times = [res[0] for res in intermediate_results]
                    avg_time = np.mean(execution_times)
                    
                    results.append((
                        float(avg_time), 
                        int(result[1]), 
                        float(np.log2(result[2])), 
                        float(np.log2(result[3]))
                    ))
                    
    with open("../results/performance_results.json", 'w') as f:
        json.dump(results, f, indent=4)

def read_results():
    with open("../results/performance_results.json", 'r') as f:
        results = json.load(f)
    return results

def group_results_by_threads(results):
    grouped = {}
    for result in results:
        thread_num = result[2]
        if thread_num not in grouped:
            grouped[thread_num] = []
            
        grouped[thread_num].append(list(result[0:2]) + [result[3]])
    return grouped

def plot_to_3d_graph(grouped_results):
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D

    output_dir = "../plots"
    os.makedirs(output_dir, exist_ok=True)

    for threads, data in grouped_results.items():
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        data_np = np.array(data)
        x = data_np[:, 1]  / np.min(data_np[:, 1]) # n
        y = data_np[:, 2]  - 20 # max_memory in MB 
        z = data_np[:, 0]  # execution time

        ax.scatter(x, y, z, label=f'Threads : {2 ** threads}')

        ax.set_xlabel(f'Input Size (n) *1e{np.log10(np.min(data_np[:, 1]))}')
        ax.set_ylabel('Max Memory (log2)')
        ax.set_zlabel('Execution Time (s)')
        ax.set_title(f'Number of threads  = {2 ** threads}')
        ax.legend()

        file_path = os.path.join(output_dir, f"threads_{2 ** threads}.png")
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

    # Command 3: Plot 
    parser_plot = subparsers.add_parser("plot", help="Read JSON results and generate 3D plots")

    args = parser.parse_args()

    if args.command == "generate":
        print(f"Generating {args.num} samples from 10^{args.min} to 10^{args.max}...")
        os.makedirs("../samples", exist_ok=True)
        generate_samples(args.min, args.max, args.num)
        print("Done.")

    elif args.command == "simulate":
        print(f"Running simulator (Threads limit: 2^{args.threads}, Memory limit: 2^{args.memory})...")
        run_simulator(args.threads, args.memory)
        print("Simulations complete. Data saved to ../results/performance_results.json.")

    elif args.command == "plot":
        print("Reading results and rendering plots...")
        results = read_results()
        grouped = group_results_by_threads(results)
        plot_to_3d_graph(grouped)

if __name__ == "__main__":
    main()
import os
import json
import argparse
import numpy as np


def read_results(file_path):
    with open(file_path, 'r') as f:
        data = json.load(f)
    return data


def normalize_scores(score_counts):
    scores = np.arange(25)
    counts = np.array([score_counts.get(str(score), 0) for score in scores], dtype=np.int64)
    total = counts.sum()
    if total == 0:
        percentages = np.zeros_like(counts, dtype=float)
    else:
        percentages = counts.astype(float) / total * 100.0
    return scores, counts, percentages


def plot_bar(scores, percentages, file_path, title):
    import matplotlib
    matplotlib.use('Qt5Agg')
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar(scores, percentages, color='tab:blue', edgecolor='black')
    ax.set_xlabel('Score')
    ax.set_ylabel('Percentage of Games (%)')
    ax.set_title(title)
    ax.set_xticks(scores)
    ax.set_ylim(0, max(percentages.max() * 1.1, 1.0))
    ax.grid(axis='y', linestyle='--', alpha=0.4)

    plt.tight_layout()
    plt.savefig(file_path, bbox_inches='tight', dpi=300)
    print(f"Saved bar chart: {file_path}")
    plt.close(fig)


def plot_circle(scores, percentages, file_path, title):
    import matplotlib
    matplotlib.use('Qt5Agg')
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(10, 10))

    nonzero_mask = percentages > 0.0
    visible_scores = scores[nonzero_mask]
    visible_percentages = percentages[nonzero_mask]

    labels = [str(score) for score in visible_scores]
    sizes = visible_percentages.tolist()
    explode = [0.05 if pct == max(sizes) else 0.0 for pct in sizes]

    wedges, texts, autotexts = ax.pie(
        sizes,
        labels=labels,
        autopct='%1.1f%%',
        startangle=90,
        pctdistance=0.75,
        explode=explode,
        wedgeprops={'linewidth': 0.5, 'edgecolor': 'white'}
    )
    ax.set_title(title)
    centre_circle = plt.Circle((0, 0), 0.55, fc='white')
    fig.gca().add_artist(centre_circle)

    plt.tight_layout()
    plt.savefig(file_path, bbox_inches='tight', dpi=300)
    print(f"Saved circle chart: {file_path}")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description='Analyze and plot score distributions from a bin_results.json file.')
    parser.add_argument('--input-file', '-i', type=str, required=True, help='Path to a bin_results.json file')
    parser.add_argument('--output-dir', '-o', type=str, default='../plots', help='Directory to save generated plots')
    parser.add_argument('--plot-type', '-t', choices=['bar', 'circle'], default='bar', help='Chart type to generate')
    parser.add_argument('--show', action='store_true', help='Show the plot interactively after saving')
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    score_counts = read_results(args.input_file)
    scores, counts, percentages = normalize_scores(score_counts)

    base_name = os.path.splitext(os.path.basename(args.input_file))[0]
    if args.plot_type == 'bar':
        output_path = os.path.join(args.output_dir, f"{base_name}_score_distribution_bar.png")
    else:
        output_path = os.path.join(args.output_dir, f"{base_name}_score_distribution_circle.png")

    title = f"Score Distribution ({base_name})"
    if args.plot_type == 'bar':
        plot_bar(scores, percentages, output_path, title)
    else:
        plot_circle(scores, percentages, output_path, title)

    if args.show:
        import matplotlib
        matplotlib.use('Qt5Agg')
        import matplotlib.pyplot as plt
        plt.show()


if __name__ == '__main__':
    main()

import json
import matplotlib.pyplot as plt
from pathlib import Path

JSON_PATH = Path("unified_simulation_data.json")
OUTPUT_DIR = Path("plots")
OUTPUT_DIR.mkdir(exist_ok=True)

STRATEGY_COLORS = {
    "shortest": "orange",
    "project": "royalblue",
    "farthest": "green",
}

def log(msg: str):
    print(f"✅ {msg}")

def load_data(path: Path):
    log(f"Loading data from {path}")
    with open(path, "r") as f:
        return json.load(f)

def save_plot(fig, filename: str):
    filepath = OUTPUT_DIR / f"{filename}.png"
    fig.savefig(filepath, dpi=300, bbox_inches="tight")
    log(f"Saved: {filepath}")

def get_people_series(strategy_block):
    people = [
        {
            "enteringTimestamp": p["enteringTimestamp"],
            "actualWaitTime": p["actualWaitTime"],
            "expectedWaitTime": p["expectedWaitTime"],
        }
        for p in strategy_block["people"].values()
    ]
    people.sort(key=lambda p: p["enteringTimestamp"])
    return people

def extract_expected_vs_actual_curves(strategy_block):
    ppl = get_people_series(strategy_block)
    xs = list(range(1, len(ppl) + 1))
    expected = [p["expectedWaitTime"] for p in ppl]
    actual = [p["actualWaitTime"] for p in ppl]
    return xs, expected, actual

def compute_global_wait_ylim(data):
    """Computes a shared Y-axis limit for all wait-time graphs (expected/actual)."""
    max_val = 0
    for block in data["strategies"].values():
        _, expected, actual = extract_expected_vs_actual_curves(block)
        if expected:
            max_val = max(max_val, max(expected))
        if actual:
            max_val = max(max_val, max(actual))
    return 0, max_val * 1.05 if max_val > 0 else 1.0

def plot_actual_wait_all_strategies(data, y_lim):
    log("Plotting: graph1_actual_wait_all_strategies")
    strategies = data["strategies"]

    fig, ax = plt.subplots()
    for name, block in strategies.items():
        ppl = get_people_series(block)
        xs = list(range(1, len(ppl) + 1))
        ys = [p["actualWaitTime"] for p in ppl]
        ax.plot(xs, ys, label=name, color=STRATEGY_COLORS.get(name), linewidth=2)

    ax.set_xlabel("Person index (arrival order)")
    ax.set_ylabel("Actual wait time (seconds)")
    ax.set_ylim(y_lim)
    ax.set_title("Actual wait time vs person index (all strategies)")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.3)
    save_plot(fig, "graph1_actual_wait_all_strategies")

def plot_expected_vs_actual_per_strategy(data, y_lim):
    log("Plotting: graph2_expected_vs_actual_<strategy>")
    for name, block in data["strategies"].items():
        xs, expected, actual = extract_expected_vs_actual_curves(block)
        fig, ax = plt.subplots()
        ax.plot(xs, expected, label="Expected", linewidth=2)
        ax.plot(xs, actual, label="Actual", linewidth=2)
        ax.set_xlabel("Person index (arrival order)")
        ax.set_ylabel("Wait time (seconds)")
        ax.set_ylim(y_lim)
        ax.set_title(f"Expected vs Actual wait time - {name}")
        ax.legend()
        ax.grid(True, linestyle="--", alpha=0.3)
        save_plot(fig, f"graph2_expected_vs_actual_{name}")

def plot_historical_avg_actual_wait_bar(data):
    log("Plotting: graph3_historical_avg_actual_wait")
    names, vals, colors = [], [], []
    for name, block in data["strategies"].items():
        names.append(name)
        vals.append(block["overallStats"]["historicalAvgActualWait"])
        colors.append(STRATEGY_COLORS.get(name))
    fig, ax = plt.subplots()
    ax.bar(names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha='center', va='bottom')
    ax.set_ylabel("Historical Avg Actual Wait (seconds)")
    ax.set_title("Historical Avg Actual Wait per Strategy")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph3_historical_avg_actual_wait")

def plot_max_actual_avg_wait_time_bar(data):
    log("Plotting: graph4_max_actual_avg_wait")
    names, vals, colors = [], [], []
    for name, block in data["strategies"].items():
        max_val = max(l["actualAvgWaitTime"] for l in block["lineStats"].values())
        names.append(name)
        vals.append(max_val)
        colors.append(STRATEGY_COLORS.get(name))
    fig, ax = plt.subplots()
    ax.bar(names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha='center', va='bottom')
    ax.set_ylabel("Max actualAvgWaitTime across lines (seconds)")
    ax.set_title("Peak Average Wait (busiest line) per Strategy")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph4_max_actual_avg_wait")

def main():
    data = load_data(JSON_PATH)
    y_lim = compute_global_wait_ylim(data)
    plot_actual_wait_all_strategies(data, y_lim)
    plot_expected_vs_actual_per_strategy(data, y_lim)
    plot_historical_avg_actual_wait_bar(data)
    plot_max_actual_avg_wait_time_bar(data)
    plt.show()

if __name__ == "__main__":
    main()

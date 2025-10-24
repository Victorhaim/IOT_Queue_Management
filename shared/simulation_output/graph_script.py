import json
import matplotlib.pyplot as plt
from pathlib import Path

# ========================
# Config
# ========================

JSON_PATH = Path("unified_simulation_data.json")
OUTPUT_DIR = Path("plots")
OUTPUT_DIR.mkdir(exist_ok=True)

STRATEGY_COLORS = {
    "shortest": "orange",
    "project": "royalblue",
    "farthest": "green",
}

# ========================
# Helpers
# ========================

def load_data(path: Path):
    with open(path, "r") as f:
        return json.load(f)

def save_plot(fig, filename: str):
    """שומר גרף כתמונה בפורמט PNG בתוך plots/"""
    filepath = OUTPUT_DIR / f"{filename}.png"
    fig.savefig(filepath, dpi=300, bbox_inches="tight")
    print(f"✅ Saved: {filepath}")

def get_people_series(strategy_block):
    people = [
        {
            "personId": pid,
            "enteringTimestamp": p["enteringTimestamp"],
            "actualWaitTime": p["actualWaitTime"],
            "expectedWaitTime": p["expectedWaitTime"],
        }
        for pid, p in strategy_block["people"].items()
    ]
    people.sort(key=lambda p: p["enteringTimestamp"])
    return people

def extract_actual_wait_curve(strategy_block):
    ppl = get_people_series(strategy_block)
    xs = list(range(1, len(ppl) + 1))
    ys = [p["actualWaitTime"] for p in ppl]
    return xs, ys

def extract_expected_vs_actual_curves(strategy_block):
    ppl = get_people_series(strategy_block)
    xs = list(range(1, len(ppl) + 1))
    expected = [p["expectedWaitTime"] for p in ppl]
    actual = [p["actualWaitTime"] for p in ppl]
    return xs, expected, actual

def get_historical_avg_actual_wait(strategy_block):
    return strategy_block["overallStats"]["historicalAvgActualWait"]

def get_max_actual_avg_wait_time(strategy_block):
    max_val = 0.0
    for line_data in strategy_block["lineStats"].values():
        max_val = max(max_val, line_data["actualAvgWaitTime"])
    return max_val

# ========================
# Plots
# ========================

def plot_actual_wait_all_strategies(data):
    strategies = data["strategies"]
    fig, ax = plt.subplots()
    for name, block in strategies.items():
        xs, ys = extract_actual_wait_curve(block)
        ax.plot(xs, ys, label=name, color=STRATEGY_COLORS.get(name), linewidth=2)
    ax.set_xlabel("Person index (arrival order)")
    ax.set_ylabel("Actual wait time (seconds)")
    ax.set_title("Actual wait time vs person index (all strategies)")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.3)
    save_plot(fig, "graph1_actual_wait_all_strategies")

def plot_expected_vs_actual_per_strategy(data):
    strategies = data["strategies"]
    for name, block in strategies.items():
        xs, expected, actual = extract_expected_vs_actual_curves(block)
        fig, ax = plt.subplots()
        ax.plot(xs, expected, label="Expected", linewidth=2)
        ax.plot(xs, actual, label="Actual", linewidth=2)
        ax.set_xlabel("Person index (arrival order)")
        ax.set_ylabel("Wait time (seconds)")
        ax.set_title(f"Expected vs Actual wait time - {name}")
        ax.legend()
        ax.grid(True, linestyle="--", alpha=0.3)
        save_plot(fig, f"graph2_expected_vs_actual_{name}")

def plot_historical_avg_actual_wait_bar(data):
    strategies = data["strategies"]
    names = list(strategies.keys())
    vals = [get_historical_avg_actual_wait(b) for b in strategies.values()]
    colors = [STRATEGY_COLORS.get(n) for n in names]

    fig, ax = plt.subplots()
    ax.bar(names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha='center', va='bottom', fontsize=9)
    ax.set_ylabel("Historical average actual wait (seconds)")
    ax.set_title("Historical Avg Actual Wait per Strategy")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph3_historical_avg_actual_wait")

def plot_max_actual_avg_wait_time_bar(data):
    strategies = data["strategies"]
    names = list(strategies.keys())
    vals = [get_max_actual_avg_wait_time(b) for b in strategies.values()]
    colors = [STRATEGY_COLORS.get(n) for n in names]

    fig, ax = plt.subplots()
    ax.bar(names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha='center', va='bottom', fontsize=9)
    ax.set_ylabel("Max actualAvgWaitTime across lines (seconds)")
    ax.set_title("Peak Average Wait (busiest line) per Strategy")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph4_max_actual_avg_wait")

# ========================
# Main
# ========================

def main():
    data = load_data(JSON_PATH)

    plot_actual_wait_all_strategies(data)
    plot_expected_vs_actual_per_strategy(data)
    plot_historical_avg_actual_wait_bar(data)
    plot_max_actual_avg_wait_time_bar(data)

    plt.show()

if __name__ == "__main__":
    main()

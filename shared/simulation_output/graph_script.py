import json
import csv
import matplotlib.pyplot as plt
from pathlib import Path

# ======================================
# Config
# ======================================

JSON_PATH = Path("unified_simulation_data.json")
OUTPUT_DIR = Path("plots")
OUTPUT_DIR.mkdir(exist_ok=True)

Y_CEILING_SECONDS = 5000  # max displayed wait time

SIM_COLORS = {
    "simulation_project": "royalblue",
    "simulation_farthest": "green",
    "simulation_shortest": "purple",
    # ESP32 intentionally omitted
}

def get_color_for_sim(sim_name: str):
    return SIM_COLORS.get(sim_name, "gray")

def log(msg: str):
    print(f"✅ {msg}")

# ======================================
# Data handling
# ======================================

def load_data(path: Path):
    log(f"Loading data from {path}")
    with open(path, "r") as f:
        return json.load(f)

def save_plot(fig, filename: str):
    filepath = OUTPUT_DIR / f"{filename}.png"
    fig.savefig(filepath, dpi=300, bbox_inches="tight")
    log(f"Saved: {filepath}")

def get_all_simulation_names(data):
    """
    Filter only relevant simulations — ditch simulation_ESP32 entirely.
    """
    return [
        name for name in data.keys()
        if name.startswith("simulation_") and name != "simulation_ESP32"
    ]

def get_people_series(sim_block):
    ppl_raw = sim_block["people"]
    ppl_list = []
    for pid, p in ppl_raw.items():
        ppl_list.append({
            "personId": pid,
            "enteringTimestamp": p.get("enteringTimestamp", 0),
            "actualWaitTime": p.get("actualWaitTime", 0),
            "expectedWaitTime": p.get("expectedWaitTime", 0),
            "exitingTimestamp": p.get("exitingTimestamp", 0),
            "lineNumber": p.get("lineNumber", None),
            "hasExited": p.get("hasExited", False),
        })
    ppl_list.sort(key=lambda row: row["enteringTimestamp"])
    return ppl_list

def extract_curves_expected_vs_actual(sim_block):
    ppl = get_people_series(sim_block)
    xs = list(range(1, len(ppl) + 1))
    expected_vals = [p["expectedWaitTime"] for p in ppl]
    actual_vals = [p["actualWaitTime"] for p in ppl]
    return xs, expected_vals, actual_vals

def compute_global_wait_ylim(data):
    max_val = 0.0
    for sim_name in get_all_simulation_names(data):
        sim_block = data[sim_name]
        _, expected_list, actual_list = extract_curves_expected_vs_actual(sim_block)
        if expected_list:
            max_val = max(max_val, max(expected_list))
        if actual_list:
            max_val = max(max_val, max(actual_list))
    upper = max_val * 1.05 if max_val > 0 else 1.0
    if upper > Y_CEILING_SECONDS:
        upper = Y_CEILING_SECONDS
    return (0, upper)

def get_historical_avg_actual_wait(sim_block):
    return sim_block["overallStats"]["historicalAvgActualWait"]

def get_max_queue_wait(sim_block):
    max_val = 0.0
    queues = sim_block.get("queues", {})
    for q in queues.values():
        wait_est = q.get("estimatedWaitForNewPerson", 0.0)
        if wait_est > max_val:
            max_val = wait_est
    return max_val

def export_json_to_csv(data, output_csv_path="unified_simulation_data.csv"):
    log("Exporting CSV...")
    rows = []
    for sim_name in get_all_simulation_names(data):
        sim_block = data[sim_name]
        for person_id, p in sim_block["people"].items():
            rows.append({
                "simulationName": sim_name,
                "personId": person_id,
                "enteringTimestamp": p.get("enteringTimestamp", ""),
                "exitingTimestamp": p.get("exitingTimestamp", ""),
                "lineNumber": p.get("lineNumber", ""),
                "expectedWaitTime": p.get("expectedWaitTime", ""),
                "actualWaitTime": p.get("actualWaitTime", ""),
                "hasExited": p.get("hasExited", ""),
            })
    with open(output_csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "simulationName",
                "personId",
                "enteringTimestamp",
                "exitingTimestamp",
                "lineNumber",
                "expectedWaitTime",
                "actualWaitTime",
                "hasExited",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)
    log(f"CSV written to {output_csv_path}")

# ======================================
# Plotting
# ======================================

def plot_actual_wait_all_sims(data, y_lim):
    log("Plotting: graph1_actual_wait_all_sims")
    fig, ax = plt.subplots()
    for sim_name in get_all_simulation_names(data):
        sim_block = data[sim_name]
        ppl = get_people_series(sim_block)
        xs = list(range(1, len(ppl) + 1))
        ys = [p["actualWaitTime"] for p in ppl]
        ax.plot(xs, ys, label=sim_name,
                color=get_color_for_sim(sim_name), linewidth=2)
    ax.set_xlabel("Person index (arrival order)")
    ax.set_ylabel("Actual wait time (seconds)")
    ax.set_ylim(y_lim)
    ax.set_title("Actual wait time vs person index (all simulations)")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.3)
    save_plot(fig, "graph1_actual_wait_all_sims")

def plot_expected_vs_actual_per_sim(data, y_lim):
    log("Plotting: graph2_expected_vs_actual_<simulation>")
    for sim_name in get_all_simulation_names(data):
        sim_block = data[sim_name]
        xs, exp_vals, act_vals = extract_curves_expected_vs_actual(sim_block)
        fig, ax = plt.subplots()
        ax.plot(xs, exp_vals, label="Expected", linewidth=2)
        ax.plot(xs, act_vals, label="Actual", linewidth=2)
        ax.set_xlabel("Person index (arrival order)")
        ax.set_ylabel("Wait time (seconds)")
        ax.set_ylim(y_lim)
        ax.set_title(f"Expected vs Actual wait time - {sim_name}")
        ax.legend()
        ax.grid(True, linestyle="--", alpha=0.3)
        save_plot(fig, f"graph2_expected_vs_actual_{sim_name}")

def plot_historical_avg_actual_wait_bar(data):
    log("Plotting: graph3_historical_avg_actual_wait")
    sim_names = get_all_simulation_names(data)
    vals = []
    colors = []
    for sim_name in sim_names:
        sim_block = data[sim_name]
        vals.append(get_historical_avg_actual_wait(sim_block))
        colors.append(get_color_for_sim(sim_name))
    fig, ax = plt.subplots()
    ax.bar(sim_names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha="center", va="bottom", fontsize=9)
    ax.set_ylabel("Historical Avg Actual Wait (seconds)")
    ax.set_title("Historical Avg Actual Wait per Simulation")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph3_historical_avg_actual_wait")

def plot_max_actual_avg_wait_time_bar(data):
    log("Plotting: graph4_max_queue_wait")
    sim_names = get_all_simulation_names(data)
    vals = []
    colors = []
    for sim_name in sim_names:
        sim_block = data[sim_name]
        vals.append(get_max_queue_wait(sim_block))
        colors.append(get_color_for_sim(sim_name))
    fig, ax = plt.subplots()
    ax.bar(sim_names, vals, color=colors)
    for i, v in enumerate(vals):
        ax.text(i, v, f"{v:.2f}", ha="center", va="bottom", fontsize=9)
    ax.set_ylabel("Max estimated wait across queues (seconds)")
    ax.set_title("Peak queue wait per Simulation (worst line right now)")
    ax.grid(axis="y", linestyle="--", alpha=0.3)
    save_plot(fig, "graph4_max_queue_wait")

# ======================================
# Main
# ======================================

def main():
    data = load_data(JSON_PATH)
    export_json_to_csv(data)
    y_lim = compute_global_wait_ylim(data)
    plot_actual_wait_all_sims(data, y_lim)
    plot_expected_vs_actual_per_sim(data, y_lim)
    plot_historical_avg_actual_wait_bar(data)
    plot_max_actual_avg_wait_time_bar(data)
    log("Showing all figures...")
    plt.show()

if __name__ == "__main__":
    main()

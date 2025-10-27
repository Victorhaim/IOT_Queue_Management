import json
import csv
import matplotlib.pyplot as plt
from pathlib import Path
import glob
import os

# ======================================
# Config
# ======================================


# Auto-detect the most recent Firebase export file
def find_latest_firebase_export():
    """Find the most recent Firebase export file in the current directory"""
    patterns = [
        "firebase_export_*.json",
        "iot-queue-management-default-rtdb-export*.json",
    ]

    all_files = []
    for pattern in patterns:
        all_files.extend(glob.glob(pattern))

    if not all_files:
        raise FileNotFoundError(
            "No Firebase export files found. Please ensure you have a file matching 'firebase_export_*.json' or 'iot-queue-management-default-rtdb-export*.json'"
        )

    # Sort by modification time (most recent first)
    all_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    return Path(all_files[0])


JSON_PATH = find_latest_firebase_export()
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
    try:
        # Try UTF-8 with BOM first (PowerShell default), then regular UTF-8
        encodings_to_try = ["utf-8-sig", "utf-8", "utf-16"]

        data = None
        for encoding in encodings_to_try:
            try:
                with open(path, "r", encoding=encoding) as f:
                    data = json.load(f)
                break
            except (UnicodeDecodeError, json.JSONDecodeError) as e:
                if encoding == encodings_to_try[-1]:  # Last encoding attempt
                    raise e
                continue

        if data is None:
            raise ValueError("Could not decode file with any supported encoding")

        # Validate the data structure
        if not isinstance(data, dict):
            raise ValueError("JSON file does not contain a dictionary at root level")

        # Check for simulation data
        simulation_keys = [key for key in data.keys() if key.startswith("simulation_")]
        if not simulation_keys:
            raise ValueError(
                "No simulation data found in JSON file. Expected keys starting with 'simulation_'"
            )

        log(f"Found {len(simulation_keys)} simulations: {', '.join(simulation_keys)}")

        # Validate each simulation has required structure
        for sim_key in simulation_keys:
            sim_data = data[sim_key]
            if not isinstance(sim_data, dict):
                log(f"Warning: {sim_key} is not a dictionary, skipping validation")
                continue

            required_keys = ["overallStats", "people"]
            missing_keys = [key for key in required_keys if key not in sim_data]
            if missing_keys:
                log(f"Warning: {sim_key} missing keys: {missing_keys}")
            else:
                people_count = len(sim_data.get("people", {}))
                log(f"  {sim_key}: {people_count} people")

        return data

    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON format in {path}: {e}")
    except FileNotFoundError:
        raise FileNotFoundError(f"File not found: {path}")
    except Exception as e:
        raise RuntimeError(f"Error loading data from {path}: {e}")


def save_plot(fig, filename: str):
    filepath = OUTPUT_DIR / f"{filename}.png"
    fig.savefig(filepath, dpi=300, bbox_inches="tight")
    log(f"Saved: {filepath}")


def get_all_simulation_names(data):
    """
    Filter only relevant simulations — ditch simulation_ESP32 entirely.
    """
    return [
        name
        for name in data.keys()
        if name.startswith("simulation_") and name != "simulation_ESP32"
    ]


def get_people_series(sim_block):
    if "people" not in sim_block:
        log(f"Warning: No 'people' key found in simulation block")
        return []

    ppl_raw = sim_block["people"]
    if not isinstance(ppl_raw, dict):
        log(f"Warning: 'people' is not a dictionary")
        return []

    ppl_list = []
    for pid, p in ppl_raw.items():
        if not isinstance(p, dict):
            log(f"Warning: Person {pid} is not a dictionary, skipping")
            continue

        # Use safe defaults for missing fields
        person_data = {
            "personId": pid,
            "enteringTimestamp": p.get("enteringTimestamp", 0),
            "actualWaitTime": p.get("actualWaitTime", 0),
            "expectedWaitTime": p.get("expectedWaitTime", 0),
            "exitingTimestamp": p.get("exitingTimestamp", 0),
            "lineNumber": p.get("lineNumber", None),
            "hasExited": p.get("hasExited", False),
        }

        # Ensure numeric fields are actually numeric
        for field in [
            "enteringTimestamp",
            "actualWaitTime",
            "expectedWaitTime",
            "exitingTimestamp",
        ]:
            try:
                person_data[field] = (
                    float(person_data[field]) if person_data[field] is not None else 0.0
                )
            except (ValueError, TypeError):
                log(f"Warning: Invalid {field} for {pid}, using 0.0")
                person_data[field] = 0.0

        ppl_list.append(person_data)

    # Sort by entering timestamp
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
            rows.append(
                {
                    "simulationName": sim_name,
                    "personId": person_id,
                    "enteringTimestamp": p.get("enteringTimestamp", ""),
                    "exitingTimestamp": p.get("exitingTimestamp", ""),
                    "lineNumber": p.get("lineNumber", ""),
                    "expectedWaitTime": p.get("expectedWaitTime", ""),
                    "actualWaitTime": p.get("actualWaitTime", ""),
                    "hasExited": p.get("hasExited", ""),
                }
            )
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
        ax.plot(xs, ys, label=sim_name, color=get_color_for_sim(sim_name), linewidth=2)
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
    try:
        print(f"🔍 Searching for Firebase export files...")
        print(f"📁 Using file: {JSON_PATH}")

        data = load_data(JSON_PATH)

        print(f"📊 Processing data and generating plots...")
        export_json_to_csv(data)

        y_lim = compute_global_wait_ylim(data)
        print(f"📈 Y-axis limit for wait time plots: {y_lim}")

        plot_actual_wait_all_sims(data, y_lim)
        plot_expected_vs_actual_per_sim(data, y_lim)
        plot_historical_avg_actual_wait_bar(data)
        plot_max_actual_avg_wait_time_bar(data)

        log("All plots generated successfully!")
        log("Showing all figures...")
        plt.show()

    except Exception as e:
        print(f"❌ Error: {e}")
        print(f"💡 Make sure you have a Firebase export file in the current directory.")
        print(
            f"   Expected files: firebase_export_*.json or iot-queue-management-default-rtdb-export*.json"
        )
        return 1

    return 0


if __name__ == "__main__":
    main()

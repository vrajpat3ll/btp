import os
import pandas as pd
import matplotlib.pyplot as plt

# ----------- USE ONLY 3 FILES ----------------
csv_files = [
    "static-least-conn-200/associative-latency.csv",
    "static-ip-mapping-200-latency/associative-latency.csv",
    "ip-alloc-200-20/associative-latency.csv",
    "ip-alloc-200-30/lb_amf_resource.csv"
]

labels = [
    "static-mapping-200-20",
    "static-least-conn-200-20",
    "ip-alloc-200-20",
    "ip-alloc-200-30"
]
# ---------------------------------------------

# Set the value of x (e.g., 5 ms)
x =25

dfs = []

# Read CSV files and prepare the data
for file, label in zip(csv_files, labels):
    df = pd.read_csv(file)
    df["ue_count"] = range(1, len(df) + 1)  # Add a unique count for each UE
    df["dataset"] = label  # Add the dataset label to the dataframe
    
    # Limit all latencies greater than x ms to x ms
    df["latency_ms"] = df["latency_ms"].clip(upper=x)
    
    dfs.append(df)

# Combine all dataframes into one
combined = pd.concat(dfs, ignore_index=True)

# Plotting the graph
plt.figure(figsize=(12, 6))

for label in labels:
    subset = combined[combined["dataset"] == label]
    plt.plot(
        subset["ue_count"],
        subset["latency_ms"],
        marker='o',  # Use only markers (dots)
        linestyle='',  # No line connecting the dots
        label=label
    )

# Set title and labels
plt.title(f"Latency vs ue_count")
plt.xlabel("UE_id")
plt.ylabel("Latency (ms)")
plt.grid(True)
plt.legend()
plt.tight_layout()

# ----------- SAVE THE GRAPH ----------------
# Specify the directory and filename for saving
save_path = f"Graphs/Graphs-200/latency/latency.png"  # File name with extension

# Ensure the directory exists
save_dir = os.path.dirname(save_path)
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

# Save the figure
plt.savefig(save_path)

# Optionally, show the plot
plt.show()

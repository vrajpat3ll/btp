import subprocess
from datetime import datetime
import csv
import os
import argparse
from pathlib import Path

LOG_DIR = Path(".") / "data" / "logs"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--interval", "-i", type=float, default=0.5)
    parser.add_argument("--pod", default="lb-0", help="Pod name")
    parser.add_argument("--namespace", "-n", default="loadbalancer", help="Namespace")
    return parser.parse_args()


def find_latest_timestamp(logs_dir: Path = LOG_DIR):
    return None
    try:
        entries = os.listdir(logs_dir)
    except FileNotFoundError:
        return None

    candidates = []
    for name in entries:
        path = os.path.join(logs_dir, name)
        if not os.path.isdir(path):
            continue
        try:
            dt = datetime.strptime(name, "%Y-%m-%d_%H%M%S")
        except ValueError:
            continue
        candidates.append((dt, name))

    if not candidates:
        return None

    candidates.sort()
    return candidates[-1][1]


args = parse_args()

timestamp_dir = find_latest_timestamp()
if timestamp_dir is None:
    timestamp_dir = datetime.now().strftime("%Y-%m-%d_%H%M%S")

LOG_DIR = LOG_DIR / timestamp_dir
os.makedirs(LOG_DIR, exist_ok=True)

LOG_FILE = LOG_DIR / "resource-utilization.csv"

# Write CSV header
with open(LOG_FILE, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["Timestamp", "CPU(m)", "Memory(Mi)"])

print(f"Monitoring pod: {args.pod} in namespace {args.namespace}")
print(f"Logging to: {LOG_FILE}")
print("Press Ctrl+C to stop.\n")

# Monitor loop
try:
    while True:
        # Run kubectl top
        cmd = ["sudo", "kubectl", "top", "pod", args.pod, "-n", args.namespace, "--no-headers"]

        try:
            output = subprocess.check_output(cmd, text=True).strip()
            # Example output: "lb-0   10m   17Mi"
            parts = output.split()
            cpu_m = parts[1]  # e.g., "10m"
            mem_mi = parts[2]  # e.g., "17Mi"

        except subprocess.CalledProcessError:
            cpu_m = "NA"
            mem_mi = "NA"

        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

        print(ts, cpu_m, mem_mi)

        # Append to CSV
        with open(LOG_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([ts, cpu_m, mem_mi])

        # Sleep manually
        import time

        time.sleep(args.interval)

except KeyboardInterrupt:
    print("\nStopped monitoring.")

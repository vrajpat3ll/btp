import subprocess
from datetime import datetime
import csv
import os
import argparse
from pathlib import Path
import time

LOG_DIR = Path(".") / "data" / "logs"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--interval", "-i", type=float, default=0.5)
    parser.add_argument("--lb", default="lb-0", help="Load balancer pod name")
    parser.add_argument("--namespace_lb", default="loadbalancer", help="LB namespace")
    parser.add_argument("--namespace_amf", default="open5gs", help="AMF namespace")
    return parser.parse_args()


def get_usage_from_grep(pattern, namespace):
    """
    Runs: kubectl top pods -n <ns> | grep <pattern>
    Returns (cpu_m, mem_mi) or (0,0) if not found.
    """

    try:
        cmd = f"sudo kubectl top pods -n {namespace} --no-headers | grep {pattern}"
        output = subprocess.check_output(cmd, text=True, shell=True).strip()

        # Expected format: "<podname> <cpu> <mem>"
        parts = output.split()
        cpu = parts[1].replace("m", "")
        mem = parts[2].replace("Mi", "")
        return int(cpu), int(mem)

    except subprocess.CalledProcessError:
        return 0, 0


def find_latest_timestamp(logs_dir: Path = LOG_DIR):
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

LOG_FILE = LOG_DIR / "lb_amf_resource.csv"

# ----- CSV HEADER -----
header = ["Timestamp", "LB-CPU(m)", "LB-MEM(Mi)"]
for i in range(1, 6):  # AMF-1 to AMF-5
    header += [f"amf-{i}-CPU(m)", f"amf-{i}-MEM(Mi)"]
header += ["TOTAL-AMF-CPU", "TOTAL-AMF-MEM"]

with open(LOG_FILE, "w", newline="") as f:
    csv.writer(f).writerow(header)

print("Monitoring load balancer + AMFs (1..5)")
print(f"Logging to: {LOG_FILE}")
print("Press Ctrl+C to stop\n")

# ---------------------------- MAIN LOOP ----------------------------
try:
    while True:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

        # Load Balancer usage
        lb_cpu, lb_mem = get_usage_from_grep(args.lb, args.namespace_lb)

        row = [timestamp, lb_cpu, lb_mem]

        total_amf_cpu = 0
        total_amf_mem = 0

        # AMF-1 to AMF-5
        for i in range(1, 6):
            pattern = f"core5g-amf-{i}"
            cpu, mem = get_usage_from_grep(pattern, args.namespace_amf)
            total_amf_cpu += cpu
            total_amf_mem += mem
            row += [cpu, mem]


        total_amf_cpu+=lb_cpu
        total_amf_mem+=lb_mem
        # Totals
        row += [total_amf_cpu, total_amf_mem]

        print(row)

        # Write to CSV
        with open(LOG_FILE, "a", newline="") as f:
            csv.writer(f).writerow(row)

        time.sleep(args.interval)

except KeyboardInterrupt:
    print("\nStopped monitoring.")

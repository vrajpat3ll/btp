import psutil
from datetime import datetime
import csv
import os
import argparse
from pathlib import Path

PID_FILE = Path("/lb") / "logs" / "tmp.pid"
LOG_DIR = Path("/lb") / "logs"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--interval", "-i", type=float, default=0.25)
    parser.add_argument(
        "--pid",
        "-p",
        type=int,
        default=None,
        help="PID to monitor; if omitted, read from /lb/logs/tmp.pid",
    )
    return parser.parse_args()


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

    # return the name corresponding to the latest datetime
    candidates.sort()
    return candidates[-1][1]


args = parse_args()

timestamp = find_latest_timestamp()
if timestamp is None:
    print(
        "No timestamped log directories found in /lb/logs; using current time as fallback."
    )
    timestamp = datetime.now().strftime("%Y-%m-%d_%H%M%S")

LOG_DIR = LOG_DIR / timestamp
dt = datetime.now().strftime("%Y-%m-%d_%H%M%S")
# LOG_FILE = LOG_DIR / f"resource-utilization-{dt}.csv"
LOG_FILE = LOG_DIR / "resource-utilization.csv"

TARGET_PID = None
if getattr(args, "pid", None) is not None:
    TARGET_PID = args.pid
else:
    if not PID_FILE.exists():
        print(f"PID file not found: {PID_FILE}")
        exit(1)

    with open(PID_FILE, "r") as f:
        try:
            TARGET_PID = int(f.read().strip())
        except ValueError:
            print("PID file is corrupted or empty.")
            exit(1)

if getattr(args, "pid", None) is not None:
    print(f"Monitoring PID {TARGET_PID} (passed via --pid) ...")
else:
    print(f"Monitoring PID {TARGET_PID} from {PID_FILE} ...")

# get process handle
try:
    process = psutil.Process(TARGET_PID)
except psutil.NoSuchProcess:
    print(f"Process with PID {TARGET_PID} does not exist.")
    exit(1)

# create timestamped logs directory if missing
os.makedirs(LOG_DIR, exist_ok=True)

# write CSV header
with open(LOG_FILE, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["Timestamp", "CPU Usage(%)", "RAM Usage(%)", "RAM Usage(MB)"])

print(f"Logging CPU & RAM usage to: {LOG_FILE}")
print("Press Ctrl+C to stop.\n")

# Monitor loop
try:
    while True:
        cpu = process.cpu_percent(interval=args.interval)
        mem_info = process.memory_info()

        ram_percent = process.memory_percent()  # RAM %
        ram_mb = mem_info.rss / (1000 * 1000)  # RAM in MB

        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

        print(timestamp, cpu, ram_percent, f"{ram_mb:.2f}")

        # append to CSV
        with open(LOG_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([timestamp, cpu, ram_percent, f"{ram_mb:.2f}"])

except KeyboardInterrupt:
    print("\nStopped monitoring.")

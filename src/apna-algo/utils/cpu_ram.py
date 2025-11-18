import psutil
import time
from datetime import datetime
import csv
import os

PID_FILE = "/lb/logs/tmp.pid"
LOG_FILE = "/lb/logs/cpu_ram_logs.csv"

# ---- Step 1: Read PID from file ----
if not os.path.exists(PID_FILE):
    print(f"PID file not found: {PID_FILE}")
    exit(1)

with open(PID_FILE, "r") as f:
    try:
        TARGET_PID = int(f.read().strip())
    except ValueError:
        print("PID file is corrupted or empty.")
        exit(1)

print(f"Monitoring PID {TARGET_PID} from {PID_FILE} ...")

# ---- Step 2: Get process handle ----
try:
    process = psutil.Process(TARGET_PID)
except psutil.NoSuchProcess:
    print(f"Process with PID {TARGET_PID} does not exist.")
    exit(1)

# ---- Step 3: Prepare CSV ----
# create logs directory if missing
os.makedirs("/lb/logs", exist_ok=True)

# write CSV header
with open(LOG_FILE, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["Timestamp", "CPU_Usage(%)", "RAM_Usage(%)", "RAM_Usage_MB"])

print(f"Logging CPU & RAM usage to: {LOG_FILE}")
print("Press Ctrl+C to stop.\n")

# ---- Step 4: Monitor loop ----
try:
    while True:
        cpu = process.cpu_percent(interval=0.25)   # CPU every 0.25 sec
        mem_info = process.memory_info()

        ram_percent = process.memory_percent()     # RAM %
        ram_mb = mem_info.rss / (1024 * 1024)      # RAM in MB

        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

        print(timestamp, cpu, ram_percent, f"{ram_mb:.2f} MB")

        # append to CSV
        with open(LOG_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([timestamp, cpu, ram_percent, f"{ram_mb:.2f}"])

except KeyboardInterrupt:
    print("\nStopped monitoring.")

#!/bin/bash
set -e

# Start cpu_ram.py in background and capture PID
python3 cpu_ram.py &
CPU_RAM_PID=$!

echo "Started cpu_ram.py with PID ${CPU_RAM_PID}"

sudo kubectl exec -n loadbalancer lb-0 -- bash -c "cd lb; ./lb"

./rungnb.sh 1-$1

sleep 10

cd tests

now=$(date +"%Y-%m-%d_%H-%M-%S")
python3 stop_ue.py \
  --trace traces/400ue-run.json \
  --log-dir ue_logs \
  --num_ues 100 \
  --events "events/100ue_events-$now.json"

echo "Stopping cpu_ram.py (PID ${CPU_RAM_PID})"
kill "${CPU_RAM_PID}"

# wait "${CPU_RAM_PID}" 2>/dev/null || true
#!/bin/bash
set -e

NUM_UES=100

# update parameters based on arguments inside loadbalancer
CONFIG_FILE="/lb/include/config.h"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--capacity)
      CAPACITY="$2"
      sudo kubectl exec -n loadbalancer lb-0 -- bash -c "sed -i \
        's/^#define DEFAULT_AMF_CAPACITY .*/#define DEFAULT_AMF_CAPACITY ${CAPACITY}/' \
        '$CONFIG_FILE'"
      shift 2
      ;;
    -h|--headroom)
      HEADROOM="$2"
      sudo kubectl exec -n loadbalancer lb-0 -- bash -c "sed -i \
        's/^#define HEADROOM_PERCENTAGE .*/#define HEADROOM_PERCENTAGE ${HEADROOM}f/' \
        '$CONFIG_FILE'"
      shift 2
      ;;
    -n|--num_ues)
      NUM_UES="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: experiments.sh [-c/--capacity] [-h/--headroom] [-n/--num_ues]"
      exit 1
      ;;
  esac
done

# Start cpu_ram.py in background and capture PID
python3 cpu_ram.py &
CPU_RAM_PID=$!
echo "Started cpu_ram.py with PID ${CPU_RAM_PID}"

cleanup() {
  echo "Stopping cpu_ram.py (PID ${CPU_RAM_PID})"
  kill "${CPU_RAM_PID}"
}

trap cleanup EXIT INT TERM

# Start loadbalancer
gnome-terminal --tab --title "LoadBalancer" -- bash -c "
    sudo kubectl exec -n loadbalancer lb-0 -- bash -c 'cd lb; make; ./lb'; 
    exec bash"

sleep 1

# Start attachment of UEs
./rungnb.sh 1-$NUM_UES

sleep 10

cd tests

# Start service (arrival+departure) of UEs
now=$(date +"%Y-%m-%d_%H-%M-%S")
python3 stop_ue.py              \
  --trace traces/400ue-run.json \
  --log-dir ue_logs             \
  --num_ues $NUM_UES            \
  --events "events/100ue_events-$now.json"

# wait "${CPU_RAM_PID}" 2>/dev/null || true
#!/bin/bash
# activate_ues.sh - Activate UEs with Poisson arrival process

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <start_ue>-<end_ue>"
    echo "Example: $0 1-50"
    exit 1
fi

range=$1
LOG_DIR="ue_logs"
mkdir -p "$LOG_DIR"

# Validate and parse range
if [[ ! "$range" =~ ^[0-9]+-[0-9]+$ ]]; then
    echo "Error: Argument must be in the format <start>-<end>"
    exit 1
fi

start=${range%-*}
end=${range#*-}

if (( start > end )); then
    echo "Error: Start index cannot be greater than end index."
    exit 1
fi

if [ ! -f "poisson_trace.json" ]; then
    echo "Error: poisson_trace.json not found. Run generate_trace.sh first."
    exit 1
fi

# Setup sudo timestamp
sudo -v
echo "Defaults timestamp_type=global,timestamp_timeout=600" | sudo tee /etc/sudoers.d/99-global-timestamp > /dev/null
sudo chmod 440 /etc/sudoers.d/99-global-timestamp

echo "Starting UE activation from $start to $end"
echo "Logs will be saved to: $LOG_DIR/"

# Read trace data
arrival_times=($(jq -r '.ues[] | .arrival_delay' poisson_trace.json))
service_times=($(jq -r '.ues[] | .service_time' poisson_trace.json))

if [ "${#arrival_times[@]}" -lt "$end" ]; then
    echo "Error: Not enough UE entries in trace (need $end, have ${#arrival_times[@]})"
    exit 1
fi

# Function to start UE with proper logging
start_ue() {
    local ue_id=$1
    local arrival_delay=$2
    local service_time=$3
    local log_file="$LOG_DIR/ue_${ue_id}.log"
    
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting UE $ue_id (arrival: ${arrival_delay}s, service: ${service_time}s)" | tee -a "$log_file"
    
    # Start UE in background and capture PID
    gnome-terminal --title="UE $ue_id" -- bash -c "
        echo '=== UE $ue_id Started ===' >> '$log_file'
        echo 'Arrival delay: ${arrival_delay}s' >> '$log_file'
        echo 'Service time: ${service_time}s' >> '$log_file'
        echo 'Start time: $(date)' >> '$log_file'
        
        # Start the UE process with tee to capture output
        sudo kubectl -n ran-simulator$ue_id exec deploy/sim5g-simulator -- \
            bash -c 'cd /root/go/src/my5G-RANTester/cmd/ && ./app ue' 2>&1 | tee -a '$log_file'
        
        echo '=== UE $ue_id Process Ended ===' >> '$log_file'
    " &
    
    local terminal_pid=$!
    echo $terminal_pid > "$LOG_DIR/ue_${ue_id}_terminal.pid"
}

# Start monitoring script in background
./monitor_ues.sh "$start" "$end" &

# Activate UEs according to Poisson process
for ((i=start; i<=end; i++)); do
    arrival_delay=${arrival_times[i-1]}
    service_time=${service_times[i-1]}
    
    echo "UE $i will start in ${arrival_delay}s and run for ${service_time}s"
    
    # Start UE in background
    start_ue "$i" "$arrival_delay" "$service_time" &
    
    sleep "$arrival_delay"
done

echo "All UE activation commands scheduled."
echo "Monitor logs in: $LOG_DIR/"
echo "Run ./stop_ues.sh $start-$end to stop all UEs gracefully"
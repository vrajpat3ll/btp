#!/bin/bash
# stop_ues.sh - Gracefully stop UEs and clean up

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <start_ue>-<end_ue>"
    exit 1
fi

range=$1
LOG_DIR="ue_logs"

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

echo "Stopping UEs from $start to $end..."

# Stop monitor script first
pkill -f "monitor_ues.sh" || true

for ((i=start; i<=end; i++)); do
    echo "Stopping UE $i..."
    
    # Method 1: Graceful stop via pod command
    if kubectl get namespace "ran-simulator$i" &> /dev/null; then
        echo "  Stopping UE process in namespace ran-simulator$i"
        
        # Send SIGTERM to UE process
        kubectl -n "ran-simulator$i" exec deploy/sim5g-simulator -- \
            bash -c 'pkill -TERM -f "app ue"' || true
        
        # Wait a bit for graceful shutdown
        sleep 2
        
        # Force kill if still running
        kubectl -n "ran-simulator$i" exec deploy/sim5g-simulator -- \
            bash -c 'pkill -KILL -f "app ue"' 2>/dev/null || true
    fi
    
    # Method 2: Close terminal windows
    terminal_pid_file="$LOG_DIR/ue_${i}_terminal.pid"
    if [ -f "$terminal_pid_file" ]; then
        terminal_pid=$(cat "$terminal_pid_file")
        kill "$terminal_pid" 2>/dev/null || true
        rm -f "$terminal_pid_file"
    fi
    
    # Log shutdown
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] UE $i stopped" >> "$LOG_DIR/ue_${i}.log"
    
    # Small delay to avoid overwhelming the system
    sleep 0.5
done

# Clean up socket connections
echo "Cleaning up network connections..."
sudo ss -tulpn | grep :8080 || true

echo "All UEs stopped. Logs available in: $LOG_DIR/"

# Generate summary report
echo "=== Experiment Summary ==="
echo "UE range: $start-$end"
echo "Total UEs: $((end - start + 1))"
echo "End time: $(date)"
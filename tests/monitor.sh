#!/bin/bash
# monitor_ues.sh - Monitor UE status and connections

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <start_ue> <end_ue>"
    exit 1
fi

start=$1
end=$2
LOG_DIR="ue_logs"
MONITOR_LOG="$LOG_DIR/monitor.log"

mkdir -p "$LOG_DIR"
echo "=== UE Monitor Started $(date) ===" >> "$MONITOR_LOG"

# Function to check UE status
check_ue_status() {
    local ue_id=$1
    local namespace="ran-simulator$ue_id"
    
    # Check if namespace exists
    if ! kubectl get namespace "$namespace" &> /dev/null; then
        echo "MISSING_NS"
        return
    fi
    
    # Check if pod is running
    local pod_status=$(kubectl -n "$namespace" get pods -o jsonpath='{.items[0].status.phase}' 2>/dev/null || echo "NO_POD")
    
    # Check for UE process in the pod
    if [ "$pod_status" = "Running" ]; then
        local ue_process=$(kubectl -n "$namespace" exec deploy/sim5g-simulator -- ps aux 2>/dev/null | grep "[a]pp ue" || echo "NOT_FOUND")
        if [ "$ue_process" != "NOT_FOUND" ]; then
            echo "RUNNING"
        else
            echo "STOPPED"
        fi
    else
        echo "$pod_status"
    fi
}

# Monitor loop
while true; do
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "=== Status check at $timestamp ===" >> "$MONITOR_LOG"
    
    for ((i=start; i<=end; i++)); do
        status=$(check_ue_status "$i")
        echo "UE $i: $status" >> "$MONITOR_LOG"
        
        # Log to individual UE log
        echo "[$timestamp] Status: $status" >> "$LOG_DIR/ue_${i}.log"
    done
    
    echo "---" >> "$MONITOR_LOG"
    sleep 30  # Check every 30 seconds
done
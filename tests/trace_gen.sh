#!/bin/bash
# generate_trace.sh - Generate Poisson arrival and departure times

set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <num_ues> <arrival_rate> <mean_service_time>"
    echo "Example: $0 100 2.0 300"
    exit 1
fi

NUM_UES=$1
ARRIVAL_RATE=$2  # arrivals per second
MEAN_SERVICE_TIME=$3  # seconds

echo "Generating trace for $NUM_UES UEs with arrival rate $ARRIVAL_RATE/s and mean service time ${MEAN_SERVICE_TIME}s"

# Use Python for more accurate Poisson distribution
python3 << EOF
import numpy as np
import json
from datetime import datetime

np.random.seed(42)  # For reproducibility

# Generate inter-arrival times (exponential distribution)
inter_arrival_times = np.random.exponential(1/$ARRIVAL_RATE, $NUM_UES)

# Generate service times (exponential distribution)  
service_times = np.random.exponential($MEAN_SERVICE_TIME, $NUM_UES)

# Calculate absolute arrival times
arrival_times = np.cumsum(inter_arrival_times)

# Save trace with metadata
trace = {
    "metadata": {
        "num_ues": $NUM_UES,
        "arrival_rate": $ARRIVAL_RATE,
        "mean_service_time": $MEAN_SERVICE_TIME,
        "generated_at": datetime.now().isoformat()
    },
    "ues": []
}

for i in range($NUM_UES):
    trace["ues"].append({
        "ue_id": i+1,
        "arrival_delay": float(inter_arrival_times[i]),
        "service_time": float(service_times[i]),
        "absolute_arrival": float(arrival_times[i])
    })

with open('poisson_trace.json', 'w') as f:
    json.dump(trace, f, indent=2)

print(f"Generated trace for {len(trace['ues'])} UEs")
print(f"Total simulation time: {arrival_times[-1]:.2f} seconds")
EOF

# Also generate simple text files for backward compatibility
python3 << EOF
import json
with open('poisson_trace.json', 'r') as f:
    trace = json.load(f)

with open('arrival_times.txt', 'w') as f:
    for ue in trace['ues']:
        f.write(f"{ue['arrival_delay']:.6f}\n")

with open('service_times.txt', 'w') as f:
    for ue in trace['ues']:
        f.write(f"{ue['service_time']:.6f}\n")

print("Text files generated: arrival_times.txt, service_times.txt")
EOF
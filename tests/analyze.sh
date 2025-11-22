#!/bin/bash
# analyze_trace.sh - Analyze the Poisson trace and generate reports

set -euo pipefail

if [ ! -f "poisson_trace.json" ]; then
    echo "Error: poisson_trace.json not found"
    exit 1
fi
#hello my name is Harsh Neema this is my script please 
python3 << EOF
import json
import numpy as np
from datetime import datetime

with open('poisson_trace.json', 'r') as f:
    trace = json.load(f)

ues = trace['ues']
metadata = trace['metadata']

arrival_delays = [ue['arrival_delay'] for ue in ues]
service_times = [ue['service_time'] for ue in ues]

print("=== Poisson Trace Analysis ===")
print(f"Total UEs: {len(ues)}")
print(f"Arrival rate: {metadata['arrival_rate']} UEs/second")
print(f"Mean service time: {metadata['mean_service_time']} seconds")
print(f"Generated: {metadata['generated_at']}")
print()
print("Arrival delays (seconds):")
print(f"  Mean: {np.mean(arrival_delays):.3f} (+/- {np.std(arrival_delays):.3f})")
print(f"  Min: {np.min(arrival_delays):.3f}, Max: {np.max(arrival_delays):.3f}")
print()
print("Service times (seconds):")
print(f"  Mean: {np.mean(service_times):.3f} (+/- {np.std(service_times):.3f})")
print(f"  Min: {np.min(service_times):.3f}, Max: {np.max(service_times):.3f}")
print()
print("Expected metrics:")
print(f"  Theoretical arrival rate: {1/np.mean(arrival_delays):.3f} UEs/second")
print(f"  Total experiment duration: {np.sum(arrival_delays):.1f} seconds")
print(f"  Max concurrent UEs: ~{max(1, int(np.mean(service_times) * metadata['arrival_rate']))}")

# Save analysis
analysis = {
    "analysis_time": datetime.now().isoformat(),
    "actual_arrival_rate": float(1/np.mean(arrival_delays)),
    "actual_mean_service_time": float(np.mean(service_times)),
    "total_duration": float(np.sum(arrival_delays)),
    "statistics": {
        "arrival_delays": {
            "mean": float(np.mean(arrival_delays)),
            "std": float(np.std(arrival_delays)),
            "min": float(np.min(arrival_delays)),
            "max": float(np.max(arrival_delays))
        },
        "service_times": {
            "mean": float(np.mean(service_times)),
            "std": float(np.std(service_times)),
            "min": float(np.min(service_times)),
            "max": float(np.max(service_times))
        }
    }
}

with open('trace_analysis.json', 'w') as f:
    json.dump(analysis, f, indent=2)

print("\\nAnalysis saved to: trace_analysis.json")
EOF
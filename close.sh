#!/bin/bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <start_gnb>-<end_gnb>"
    exit 1
fi

range=$1

if [[ ! "$range" =~ ^[0-9]+-[0-9]+$ ]]; then
    echo "Error: Argument must be in the format <start>-<end> (e.g., 10-20)"
    exit 1
fi

start=${range%-*}
end=${range#*-}

if (( start > end )); then
    echo "Error: Start index cannot be greater than end index."
    exit 1
fi

for ((i=start; i<=end; i++)); do
    bash -c "sudo kubectl delete -n ran-simulator$i  deploy/sim5g-simulator"    
done

#!/bin/bash

NAMESPACE="loadbalancer"
POD="lb-0"

# If no arguments are provided, open an interactive bash session
if [ $# -eq 0 ]; then
    sudo kubectl -n "$NAMESPACE" exec -ti "$POD" -- bash -c 'cd /lb && bash'
else
    # Otherwise, run the provided command inside the container
    sudo kubectl -n "$NAMESPACE" exec -ti "$POD" -- bash -c "$@"
fi

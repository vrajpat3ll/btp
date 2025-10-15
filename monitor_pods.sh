#!/bin/bash
if [ $(which k9s) ]; then
    k9s
fi

# Hide cursor for cleaner look
tput civis

# Restore cursor on exit
trap "tput cnorm; exit" INT

while true; do
    pods_output=$(sudo kubectl get pods --all-namespaces -o wide)

    Completed=$(echo "$pods_output" | grep Completed | wc -l)
    Unknown=$(echo "$pods_output" | grep Unknown | wc -l)
    Running=$(echo "$pods_output" | grep Running | wc -l)
    Init=$(echo "$pods_output" | grep Init | wc -l)
    ContainerCreating=$(echo "$pods_output" | grep ContainerCreating | wc -l)

    clear
    echo -e "$pods_output"
    echo -e ""
    echo "🔁 Pod Status Counts:"
    echo "    Init: $Init"
    echo " Unknown: $Unknown"
    echo " Running: $Running"
    echo "Creating: $ContainerCreating"

    sleep 1
done

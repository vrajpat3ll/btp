#!/bin/bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <start_gnb>-<end_gnb>"
    exit 1
fi

range=$1

# Validate and parse range
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

if [ ! -f r_time.txt ]; then
    echo "Error: r_time.txt not found."
    exit 1
fi

# Read delays (trim spaces)
readarray -t times < <(sed 's/[[:space:]]//g' r_time.txt)

if [ "${#times[@]}" -lt "$end" ]; then
    echo "Error: Not enough time entries in r_time.txt (need at least $end)."
    exit 1
fi

# passwordless sudo for 10 minutes
sudo -v
echo "Defaults timestamp_type=global,timestamp_timeout=600" \
  | sudo tee /etc/sudoers.d/99-global-timestamp
sudo chmod 440 /etc/sudoers.d/99-global-timestamp

for ((i=start; i<=end; i++)); do
    delay=${times[i-1]}

    gnome-terminal --tab -- bash -c "
        sudo kubectl -n ran-simulator$i exec deploy/sim5g-simulator -- \
            bash -c 'cd /root/go/src/my5G-RANTester/cmd/ && ./app ue';
        exec bash"
    
    sleep "$delay"
done

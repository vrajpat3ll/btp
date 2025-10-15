#!/bin/bash

# Usage: ./delete_my5GRanFolders.sh <start> <end>
if [ $# -ne 2 ]; then
  echo "Usage: $0 <start> <end>"
  exit 1
fi

start=$1
end=$2

deleted=()
not_found=()

for ((i=start; i<=end; i++)); do
  folder="charts/my5GRan-Tester/${i}" # from the root of repo
  if [ -d "$folder" ]; then
    rm -rf "$folder"
    deleted+=("$i")
  else
    not_found+=("$i")
  fi
done

if [ ${#deleted[@]} -gt 0 ]; then
  echo "Deleted folders numbers: [${deleted[*]}]"
else
  echo "Deleted folders numbers: None"
fi

if [ ${#not_found[@]} -gt 0 ]; then
  echo "Not found folders numbers: [${not_found[*]}]"
else
  echo "Not found folders numbers: None"
fi

#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <ue_id_or_namespace> [--use-sudo]"
  exit 1
fi

ID=$1
USE_SUDO=false
if [ "${2-}" = "--use-sudo" ]; then
  USE_SUDO=true
fi

# If argument looks numeric, convert to ran-simulator{ID}
if [[ "$ID" =~ ^[0-9]+$ ]]; then
  NS="ran-simulator${ID}"
else
  NS="$ID"
fi

SUDO=""
$USE_SUDO && SUDO="sudo"

echo "[INFO] Attempting graceful stop in namespace $NS"
$SUDO kubectl -n "$NS" exec deploy/sim5g-simulator -- bash -c "pkill -SIGTERM app || true" || true
sleep 2
echo "[INFO] Deleting deployment (force cleanup)"
$SUDO kubectl delete -n "$NS" deploy/sim5g-simulator --ignore-not-found=true || true
echo "[INFO] Done"

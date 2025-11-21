#!/bin/bash
set -euo pipefail
step() { echo -e "\e[33m==> $*\e[0m"; }

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <number_of_simulators>"
  exit 1
fi

NUM_SIMS="$1"
if ! [[ "$NUM_SIMS" =~ ^[0-9]+$ ]] || [[ "$NUM_SIMS" -le 0 ]]; then
  echo "Error: Argument must be a positive integer."
  exit 1
fi

./scripts/setup.sh $NUM_SIMS

# ./dock.sh

step "Cluster"
sudo kind create cluster --config config/config-3node.yml
sudo kubectl create -f config/multus-daemonset.yml

step "Namespaces"
sudo kubectl create ns open5gs || true
sudo kubectl create ns loadbalancer || true
for i in $(seq 1 $NUM_SIMS); do sudo kubectl create ns ran-simulator$i || true; done

step "CNI"
sudo koko -d kind-worker,eth1 -d kind-worker2,eth1q

sudo modprobe sctp
sudo kubectl create -f config/cni-install.yml

step "Open5GS"
sudo kubectl create -f config/core-5g-macvlan.yml
sudo helm -n open5gs upgrade --install core5g charts/open5gs
sudo kubectl -n open5gs get po

step "RBAC"
sudo kubectl apply -f config/service-account.yaml
sudo kubectl apply -f config/cluster-role.yaml
sudo kubectl apply -f config/cluster-role-binding.yaml

step "Loadbalancer"
sudo helm -n loadbalancer upgrade --install lb charts/loadbalancer
sudo kubectl -n loadbalancer get po

step "RAN sims"
for i in $(seq 1 $NUM_SIMS); do
  sudo helm -n ran-simulator$i upgrade --install sim5g charts/my5GRan-Tester/$i
done

# sleep 300

# cd ..
# ./restart_koko.sh
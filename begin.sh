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

./setup.sh $NUM_SIMS

# ./dock.sh
cd 5gcore-sctp-loadbalancer || { echo "dir missing"; exit 1; }

step "Cluster"
sudo kind create cluster --config config-3node.yml
sudo kubectl create -f multus-daemonset.yml

step "Namespaces"
sudo kubectl create ns open5gs || true
sudo kubectl create ns loadbalancer || true
for i in $(seq 1 $NUM_SIMS); do sudo kubectl create ns ran-simulator$i || true; done

step "CNI"
sudo curl -sLO https://github.com/redhat-nfvpe/koko/releases/download/v0.82/koko_0.82_linux_amd64
sudo chmod +x koko_0.82_linux_amd64
sudo ./koko_0.82_linux_amd64 -d kind-worker,eth1 -d kind-worker2,eth1q

sudo modprobe sctp
sudo kubectl create -f cni-install.yml

step "Open5GS"
sudo kubectl create -f core-5g-macvlan.yml
sudo helm -n open5gs upgrade --install core5g open5gs-helm-charts/
sudo kubectl -n open5gs get po

step "RBAC"
sudo kubectl apply -f service-account.yaml
sudo kubectl apply -f cluster-role.yaml
sudo kubectl apply -f cluster-role-binding.yaml

step "Loadbalancer"
sudo helm -n loadbalancer upgrade --install lb Loadbalancer-helm-chart/
sudo kubectl -n loadbalancer get po

step "RAN sims"
for i in $(seq 1 $NUM_SIMS); do
  sudo helm -n ran-simulator$i upgrade --install sim5g my5GRanTester$i-helm-chart/
done

# step "Monitor"
# ./monitor_pods.sh

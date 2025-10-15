# see in many cases, using the newer versions causes problems
# I was facing the exact same problems like you did for eg, upf issue, using restart_koko leading to unknown state etc.
# So, I ditched docker, kubectl and kind by using these commands

# sudo systemctl stop docker
# sudo apt-get purge -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
# sudo rm -rf /var/lib/docker
# sudo rm -rf /var/lib/containerd

# sudo rm -f /usr/local/bin/kubectl

# sudo rm -f /usr/local/bin/kind

# # Then I installed older versions released in late 2023 or early 2024

# sudo apt-get update
# sudo apt-get install -y ca-certificates curl gnupg
# sudo install -m 0755 -d /etc/apt/keyrings

# curl -fsSL https://download.docker.com/linux/ubuntu/gpg | \
# sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# echo \
#   "deb [arch=$(dpkg --print-architecture) \
#   signed-by=/etc/apt/keyrings/docker.gpg] \
#   https://download.docker.com/linux/ubuntu \
#   $(lsb_release -cs) stable" | \
# sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# sudo apt-get update

# sudo apt-get install -y docker-ce=5:24.0.6-1~ubuntu.22.04~jammy \
#   docker-ce-cli=5:24.0.6-1~ubuntu.22.04~jammy \
#   containerd.io

# curl -LO https://dl.k8s.io/release/v1.28.2/bin/linux/amd64/kubectl
# chmod +x kubectl
# sudo mv kubectl /usr/local/bin/

# curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
# chmod +x ./kind
# sudo mv ./kind /usr/local/bin/kind

curl -fsSL -o get_helm.sh https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3
sudo chmod +x ./get_helm.sh
./get_helm.sh
rm get_helm.sh

sudo curl -sLO https://github.com/redhat-nfvpe/koko/releases/download/v0.82/koko_0.82_linux_amd64
sudo chmod +x koko_0.82_linux_amd64
sudo mv koko_0.82_linux_amd64 /usr/local/bin/koko
# Now, it ran in the first time

# Also the restart_koko also started to work as a solution to issues like stuck pods, multus ip assignment problems etc
# Previously restart_koko was a complete failure leading to unknown state

# Try this and tell me if it works for you

# extra deps
sudo apt install jq

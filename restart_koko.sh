sudo systemctl stop docker
sudo systemctl start docker

sudo curl -LO https://github.com/redhat-nfvpe/koko/releases/download/v0.82/koko_0.82_linux_amd64
sudo chmod +x koko_0.82_linux_amd64
sudo ./koko_0.82_linux_amd64 -d kind-worker,eth1 -d kind-worker2,eth1
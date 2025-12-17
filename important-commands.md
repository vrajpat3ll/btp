# KUBECTL COMMANDS
alias describe=
kubectl get pods -A --no-headers | fzf | awk '{print $2, $1}' | xargs -n 2 sh -c 'kubectl describe pod $0 -n $1'
// interactively describe the pod you choose

// better version
sudo kubectl get pods -A --no-headers | fzf --with-nth=2,1 --preview 'sudo kubectl describe pod {2} -n {1}'

sudo kubectl get pods -A --no-headers | fzf --with-nth=2,1 --preview 'sudo kubectl logs {2} -n {1}'

# PACKET CAPTURE COMMANDS
Run this in inside lb-0
```sh
tcpdump -i net1 -w capture.pcap
```
Run this inside the host machine
```sh
sudo kubectl cp loadbalancer/lb-0:/capture.pcap  capture-one-UE.pcap
```

# TO RUN THE LOADBALANCER
```sh
sudo kubectl exec -ti -n loadbalancer lb-0 -- bash
```

# TO GET ACCESS TO ROOT FILES
```sh
sudo chown btp:btp -R data/logs
```

# TO GET LOGS FROM CLUSTER TO MACHINE
```sh
sudo kubectl cp loadbalancer/lb-0:/lb/logs  data/logs
```
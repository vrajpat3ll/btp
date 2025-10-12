# KUBECTL COMMANDS
alias describe=
kubectl get pods -A --no-headers | fzf | awk '{print $2, $1}' | xargs -n 2 sh -c 'kubectl describe pod $0 -n $1'
// interactively describe the pod you choose

// better version
sudo kubectl get pods -A --no-headers | fzf --with-nth=2,1 --preview 'sudo kubectl describe pod {2} -n {1}'

sudo kubectl get pods -A --no-headers | fzf --with-nth=2,1 --preview 'sudo kubectl logs {2} -n {1}'

# PACKET CAPTURE COMMANDS
```sh
sudo kubectl cp loadbalancer/lb-0:/capture.pcap  capture-one-UE.pcap
```
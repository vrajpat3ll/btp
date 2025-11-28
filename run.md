For static/ip-mapping
- change AMF capacity to (num_ues used for testing / 5)
  - ```
   ./lb -DDEFAULT_AMF_CAPACITY=<capacity>
  ```
- then do `./dock.sh` to create image use the tag static-ip
- then
    - ```
      ./begin.sh num_ues
    ```
- In a new terminal run `./monitor_pods.sh`
- Wait for the open5gs namespace pods to get into `running` state, and `completed` state in mongo-ue-init pod.
- Then, run `./restart_koko.sh`.
- Then, in a different terminal, run
    ```
     ./load-balancer.sh
    ```
- Run `./lb` after this.
- In another terminal. run `python3 cpu_ram.py`, this will log the load balancer pod's usage inside a text file on your host machine.
- create a new terminal window. 
  - ```
    cd tests
    python3 ./trace-gen.py --num-ues <...> --lambda-arr 0.2 --lambda-dur 0.01 --out traces/<...>.json --seed 42
    python ./run_ues.py --trace traces/<...>.json --log-dir ue_logs --events events/<...>_events.json --use-sudo
  ```
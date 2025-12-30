python3 cpu_ram.py &

# run lb properly
kubectl exec -n loadbalancer lb-0 -- bash -c "cd lb;./lb"

./rungnb.sh 1-$1

sleep 10

cd tests

now=$(date +"%Y-%m-%d_%H-%M-%S")
python3 stop_ue.py --trace traces/400ue-run.json --log-dir ue_logs --num_ues 100 --events "events/100ue_events-$now.json"
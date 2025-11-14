#!/usr/bin/env python3
"""
generate_poisson_trace.py
Generate a Poisson arrival + exponential session duration trace in JSON.

Usage:
    ./generate_poisson_trace.py --num-ues 100 --lambda-arr 0.2 --lambda-dur 0.01 --out traces/run1.json --seed 42 --namespace-prefix ran-simulator
"""
import json
import argparse
import math
import random
from datetime import datetime

def exp_sample(lmbda):
    # avoid 0 by sampling u in (0,1]
    u = 1.0 - random.random()
    return -math.log(u) / lmbda

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--num-ues", type=int, required=True)
    p.add_argument("--lambda-arr", type=float, required=True, help="arrival rate (per second)")
    p.add_argument("--lambda-dur", type=float, required=True, help="session duration rate (per second)")
    p.add_argument("--out", required=True)
    p.add_argument("--seed", type=int, default=None)
    p.add_argument("--namespace-prefix", default="ran-simulator")
    args = p.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    trace = {
        "meta": {
            "generated_at": datetime.utcnow().isoformat() + "Z",
            "num_ues": args.num_ues,
            "lambda_arrival": args.lambda_arr,
            "lambda_duration": args.lambda_dur,
            "seed": args.seed,
            "namespace_prefix": args.namespace_prefix
        },
        "ues": []
    }

    current_time = 0.0
    for i in range(1, args.num_ues + 1):
        dt = exp_sample(args.lambda_arr)
        current_time += dt
        duration = exp_sample(args.lambda_dur)
        ue = {
            "ue_id": i,
            "arrival_s": round(current_time, 3),
            "duration_s": round(duration, 3),
            # namespace -> ran-simulator{ue_id} by default
            "namespace": f"{args.namespace_prefix}{i}"
        }
        trace["ues"].append(ue)

    with open(args.out, "w+") as f:
        json.dump(trace, f, indent=2)

    print(f"Wrote trace to {args.out}")

if __name__ == "__main__":
    main()

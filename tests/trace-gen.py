#!/usr/bin/env python3
"""
trace-gen.py
Generate a Poisson arrival + exponential session duration trace in JSON.

Usage:
    python3 ./trace-gen.py --num-ues 100 --lambda-arr 0.5 --lambda-dur 0.05 --setup-time 10 --out traces/400ue-run.json --seed 42
"""

import json
import argparse
import math
import random
from datetime import datetime


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--num-ues", type=int, required=True)
    p.add_argument(
        "--lambda-arr", type=float, required=True, help="arrival rate (per second)"
    )
    p.add_argument(
        "--lambda-dur",
        type=float,
        required=True,
        help="session duration rate (per second)",
    )
    p.add_argument("--setup-time", type=float, default=20.0, help="connection setup time buffer (seconds)")
    p.add_argument("--out", required=True)
    p.add_argument("--seed", type=int, default=None)
    p.add_argument("--namespace-prefix", default="ran-simulator")
    args = p.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    print("[INFO] Generating Poisson trace…")

    trace = {
        "meta": {
            "generated_at": datetime.utcnow().isoformat() + "Z",
            "num_ues": args.num_ues,
            "lambda_arrival": args.lambda_arr,
            "lambda_duration": args.lambda_dur,
            "setup_time_s": args.setup_time,
            "seed": args.seed,
            "namespace_prefix": args.namespace_prefix,
        },
        "ues": [],
    }

    current_time = 0.0
    for i in range(1, args.num_ues + 1):
        dt = random.expovariate(args.lambda_arr)
        current_time += dt
        # Duration includes setup time + random active time
        duration = args.setup_time + random.expovariate(args.lambda_dur)
        ue = {
            "ue_id": i,
            "arrival_s": round(current_time, 3),
            "duration_s": round(duration, 3),
            "namespace": f"{args.namespace_prefix}{i}",
        }
        trace["ues"].append(ue)
        if i % 50 == 0:
            print(f"[INFO] Generated UE {i}/{args.num_ues}")

    with open(args.out, "w+") as f:
        json.dump(trace, f, indent=2)

    print(f"[INFO] Trace written to {args.out}")
    print("[OK]   Generation complete.")


if __name__ == "__main__":
    main()

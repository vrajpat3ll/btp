#!/usr/bin/env python3
"""
stop_ue.py
Stop UE processes that were started earlier using run_ue.py.
Uses the same trace file and durations to schedule shutdowns.
Generates the events.json output.

Example command:
python3 stop_ue.py --trace traces/400ue-run.json --log-dir ue_logs --num_ues 200 --events events/200ue_events.json
"""

import argparse
import json
import os
import subprocess
import threading
import sys
import time
from datetime import datetime
from pathlib import Path

lock = threading.Lock()
events = []


def iso_now():
    return datetime.utcnow().isoformat()[:-3] + "Z"


def log_info(msg):
    print(f"[INFO] {iso_now()} | {msg}", end="\n\r")


def log_warn(msg):
    print(f"[WARN] {iso_now()} | {msg}", end="\n\r")


def log_error(msg):
    print(f"[ERROR] {iso_now()} | {msg}", end="\n\r")


def stop_single_ue(ue, start_time, log_dir, use_sudo):
    ue_id = ue["ue_id"]
    ns = ue.get("namespace", f"ran-simulator{ue_id}")
    duration = ue["duration_s"]
    log_path = os.path.join(log_dir, f"ue_{ue_id}.log")

    # wait until it's time to stop
    wait = ue["arrival_s"] + duration - (time.time() - start_time)
    if wait > 0:
        time.sleep(wait)

    event = {
        "ue_id": ue_id,
        "namespace": ns,
        "scheduled_arrival_s": ue["arrival_s"],
        "scheduled_duration_s": duration,
        "scheduled_time": iso_now(),
        "actual_start_time": None,
        "actual_stop_time": None,
        "termination_method": None,
        "exit_code": None,
        "log_path": log_path,
    }

    log_info(f"UE {ue_id} | Attempting graceful shutdown")

    # event["actual_stop_time"] = iso_now()

    # # graceful
    # try:
    #     sig_cmd = [
    #         "kubectl", "-n", ns, "exec", "deploy/sim5g-simulator", "--",
    #         "bash", "-c", "pkill -SIGTERM app || true",
    #     ]
    #     if use_sudo:
    #         sig_cmd.insert(0, "sudo")
    #     subprocess.run(sig_cmd, timeout=10)
    #     event["termination_method"] = "graceful"
    # except subprocess.TimeoutExpired:
    #     log_warn(f"UE {ue_id} | SIGTERM timeout")
    #     event["termination_method"] = "graceful-timeout"

    # time.sleep(3)

    # force delete deployment for safety
    try:
        del_cmd = [
            "kubectl", "-n", ns, "delete",
            "deploy/sim5g-simulator", "--ignore-not-found=true",
        ]
        if use_sudo:
            del_cmd.insert(0, "sudo")

        output = subprocess.run(del_cmd)
        # output.check_returncode()
        event["actual_stop_time"] = iso_now()
        event["exit_code"] = output.returncode
    except Exception as e:
        log_warn(f"UE{ue_id} got issue: {e}")

    with lock:
        events.append(event)

    log_info(f"UE {ue_id} | Stopped")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--trace", required=True)
    p.add_argument("--log-dir", default="ue_logs")
    p.add_argument("--events", default="events.json")
    p.add_argument("--no-sudo", action="store_true")
    p.add_argument("--num_ues", default=100)
    args = p.parse_args()

    trace_path = Path(args.trace)
    if not trace_path.exists():
        log_error(f"Trace file not found: {trace_path}")
        sys.exit(1)

    with open(trace_path) as f:
        trace = json.load(f)

    ues = trace.get("ues", [])
    if not ues:
        log_error("No UEs in trace file")
        sys.exit(1)

    log_info(f"Loaded {len(ues)} UEs from trace file")
    use_sudo = not args.no_sudo

    start_time = time.time()
    threads = []
    i = 0
    for ue in ues:
        if i < int(args.num_ues):
            i+=1
        else: 
            break
        t = threading.Thread(
            target=stop_single_ue, 
            args=(ue, start_time, args.log_dir, use_sudo)
        )
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    events_sorted = sorted(events, key=lambda e: e["ue_id"])
    out = {
        "trace_file": str(trace_path),
        "run_generated_at": iso_now(),
        "events": events_sorted,
    }
    os.makedirs(os.path.dirname(args.events) or ".", exist_ok=True)
    with open(args.events, "w") as f:
        json.dump(out, f, indent=2)

    log_info(f"Events written to {args.events}")


if __name__ == "__main__":
    main()

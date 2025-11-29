#!/usr/bin/env python3
"""
run_ues.py
Run the UEs according to a JSON trace file.

Usage:
    python3 ./run_ue.py --trace traces/400ue.json --log-dir ue_logs --events events/400ue_events.json
    python3 ./run_ue.py --trace traces/400ue.json --log-dir ue_logs --events events/400ue_events.json --no-sudo

Notes:
 - Requires kubectl on PATH.
 - For large numbers of UEs you may want to throttle concurrency.
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


def run_ue(ue, start_time, log_dir, use_sudo):
    ue_id = ue["ue_id"]
    ns = ue.get("namespace", f"ran-simulator{ue_id}")
    arrival = ue["arrival_s"]
    duration = ue["duration_s"]
    log_path = os.path.join(log_dir, f"ue_{ue_id}.log")

    # wait until arrival (relative to script start)
    wait = arrival - (time.time() - start_time)
    if wait > 0:
        time.sleep(wait)

    event = {
        "ue_id": ue_id,
        "namespace": ns,
        "scheduled_arrival_s": arrival,
        "scheduled_duration_s": duration,
        "scheduled_time": iso_now(),
        "actual_start_time": None,
        "actual_stop_time": None,
        "termination_method": None,
        "exit_code": None,
        "log_path": log_path,
    }

    log_info(f"UE {ue_id} | Starting (namespace={ns}) -> log: {log_path}")

    os.makedirs(log_dir, exist_ok=True)
    with open(log_path, "wb") as log_file:
        cmd = [
            "kubectl",
            "-n",
            ns,
            "exec",
            "deploy/sim5g-simulator",
            "--",
            "bash",
            "-c",
            "cd /root/go/src/my5G-RANTester/cmd/ && ./app ue",
        ]

        if use_sudo:
            cmd.insert(0, "sudo")

        proc = subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT)
        event["actual_start_time"] = iso_now()

        log_info(f"UE {ue_id} | Running for {duration}s")

        # sleep for the session duration, then attempt graceful shutdown
        time.sleep(duration)

        # print(f"[{iso_now()}] Stopping UE {ue_id} (attempt graceful SIGTERM)")
        log_info(f"UE {ue_id} | Attempting graceful shutdown (SIGTERM)")

        try:
            # 1) Attempt graceful shutdown inside the pod (pkill -SIGTERM app)
            sig_cmd = [
                "kubectl",
                "-n",
                ns,
                "exec",
                "deploy/sim5g-simulator",
                "--",
                "bash",
                "-c",
                "pkill -SIGTERM app || true",
            ]
            if use_sudo:
                sig_cmd.insert(0, "sudo")
            subprocess.run(sig_cmd, timeout=10)

        except subprocess.TimeoutExpired:
            # print(f"[{iso_now()}] Warning: graceful signal timed out for UE {ue_id}")
            log_warn(f"UE {ue_id} | Graceful SIGTERM timeout")

        # allow a small grace period for the app to exit
        for _ in range(5):
            if proc.poll() is not None:
                break
            time.sleep(1)

        if proc.poll() is None:
            # Not exited yet -> delete deployment to force pod termination
            # print(f"[{iso_now()}] UE {ue_id} still running, deleting deployment")
            log_warn(f"UE {ue_id} | Still running -> deleting deployment")

            del_cmd = [
                "kubectl",
                "-n",
                ns,
                "delete",
                "deploy/sim5g-simulator",
                "--ignore-not-found=true",
            ]
            if use_sudo:
                del_cmd.insert(0, "sudo")
            subprocess.run(del_cmd, timeout=15)

            # give it another short time to exit
            for _ in range(5):
                if proc.poll() is not None:
                    break
                time.sleep(1)

        # final exit code (may be None if kubectl still hung)
        exit_code = proc.poll()
        event["actual_stop_time"] = iso_now()
        event["exit_code"] = exit_code
        if exit_code is None:
            event["termination_method"] = "unknown-still-running"
            log_warn(f"UE {ue_id} | kubectl still running, force killing")
            # try to kill the local kubectl process to clean up
            try:
                proc.kill()
                event["termination_method"] = "killed-local-kubectl"
                event["exit_code"] = proc.poll()
            except Exception:
                pass
        else:
            # determine termination method heuristically
            event["termination_method"] = (
                "graceful" if exit_code == 0 else "deleted-deployment-or-error"
            )

    # Append to global events list safely
    with lock:
        events.append(event)

    # print(f"[{iso_now()}] UE {ue_id} stopped, exit_code={event['exit_code']}")
    log_info(
        f"UE {ue_id} | Stopped | exit_code={event['exit_code']} | method={event['termination_method']}"
    )


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--trace", required=True)
    p.add_argument("--log-dir", default="ue_logs")
    p.add_argument("--events", default="events.json")
    p.add_argument(
        "--no-sudo",
        action="store_true",
        help="do NOT prepend sudo to kubectl calls (sudo is used by default)",
    )
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

    total = len(ues)
    log_info(f"Loaded {total} UEs from trace file")

    # Determine whether to use sudo (use it by default, disable with --no-sudo)
    use_sudo = not args.no_sudo

    start_time = time.time()
    threads = []
    for ue in ues:
        t = threading.Thread(
            target=run_ue, args=(ue, start_time, args.log_dir, use_sudo)
        )
        t.start()
        threads.append(t)

    # Wait for all threads to finish
    for t in threads:
        t.join()

    # Write master events file (sorted by ue_id)
    events_sorted = sorted(events, key=lambda e: e["ue_id"])
    out = {
        "trace_file": str(trace_path),
        "run_generated_at": iso_now(),
        "events": events_sorted,
    }
    os.makedirs(os.path.dirname(args.events) or ".", exist_ok=True)
    with open(args.events, "w") as f:
        json.dump(out, f, indent=2)

    log_info(f"Experiment finished. Events written to {args.events}")


if __name__ == "__main__":
    main()

#!/usr/bin/env bash
set -euo pipefail
step() { echo -e "\e[33m==> $*\e[0m"; }

usage() {
	cat <<EOF
Usage: $0 <start_id> <end_id> [--action <add|delete>] [--jobs N]

Creates Kubernetes namespaces for UEs (namespace prefix is fixed to 'ran-simulator') and
installs or removes the corresponding Helm charts from
charts/my5GRan-Tester/<id> for each id in [start_id..end_id].
You can control the action with --action <add|delete>. Default is add.

This script can run operations in parallel. Use --jobs N (default 8) to set
the maximum number of concurrent operations.

When action=add: creates namespace (if missing) and runs 'helm upgrade --install'.
When action=delete: uninstalls the Helm release 'sim5g' and deletes the namespace.

Examples:
	$0 1 5
	$0 3 3 --action delete --jobs 12
EOF
	exit 1
}

if [[ $# -lt 2 ]]; then
	usage
fi

START="$1"
END="$2"
shift 2

NS_PREFIX="ran-simulator"
ACTION="add" # default: add (create/install)
JOBS=8

while [[ $# -gt 0 ]]; do
	case "$1" in
		--action|-a)
			ACTION="$2"
			shift 2
			;;
		--jobs|-j)
			JOBS="$2"
			shift 2
			;;
		*)
			echo "Unknown option: $1" >&2
			usage
			;;
	esac
done

if [[ "$ACTION" != "add" && "$ACTION" != "delete" ]]; then
	echo "Invalid action: $ACTION. Use 'add' or 'delete'." >&2
	exit 2
fi

if ! [[ "$START" =~ ^[0-9]+$ ]] || ! [[ "$END" =~ ^[0-9]+$ ]]; then
	echo "start_id and end_id must be positive integers." >&2
	exit 2
fi

if (( START > END )); then
	echo "start_id must be <= end_id" >&2
	exit 2
fi

# validate JOBS and initialize tracking
if ! [[ "$JOBS" =~ ^[0-9]+$ ]] || (( JOBS < 1 )); then
	echo "Invalid jobs value: $JOBS. Must be a positive integer." >&2
	exit 2
fi

# background job tracking
pids=()
failures=0

for i in $(seq "$START" "$END"); do
	ns="${NS_PREFIX}${i}"

	# build the commands for this id as a grouped background job
	if [[ "$ACTION" == "add" ]]; then
		cmds=(
			"step 'Creating namespace: $ns'"
			"sudo kubectl create ns \"$ns\" || true"
			"chart_dir=charts/my5GRan-Tester/$i"
			"if [[ ! -d \"\$chart_dir\" ]]; then echo \"Warning: chart directory \$chart_dir not found — skipping helm install for id=$i\" >&2; exit 0; fi"
			"step 'Installing/upgrading Helm release sim5g in namespace $ns from \$chart_dir'"
			"sudo helm -n \"$ns\" upgrade --install sim5g \"\$chart_dir\""
		)
	else
		cmds=(
			"step \"Uninstalling Helm release 'sim5g' in namespace $ns (if present)\""
			"sudo helm -n \"$ns\" uninstall sim5g || true"
			"step \"Deleting namespace: $ns\""
			"sudo kubectl delete ns \"$ns\" --ignore-not-found || true"
		)
	fi

	# start background job
	(
		set -e
		for c in "${cmds[@]}"; do
			eval "$c"
		done
	) &
	pid=$!
	pids+=("$pid")

	# throttle: if we've reached JOBS, wait for at least one to finish
	while (( ${#pids[@]} >= JOBS )); do
		if wait -n; then
			:
		else
			failures=$((failures+1))
		fi
		# prune finished pids
		newp=()
		for pid in "${pids[@]}"; do
			if kill -0 "$pid" 2>/dev/null; then
				newp+=("$pid")
			fi
		done
		pids=("${newp[@]}")
	done
done

# wait for remaining background jobs and collect failures
for pid in "${pids[@]}"; do
	if wait "$pid"; then
		:
	else
		failures=$((failures+1))
	fi
done

if (( failures > 0 )); then
	echo "Completed with $failures failed job(s)" >&2
	exit 1
fi

step "Done."

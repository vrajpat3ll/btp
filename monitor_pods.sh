#!/bin/bash
[ "$(which k9s)" ] && k9s

tput civis
trap "tput cnorm; exit" INT

LOG_FILE="monitor.log"

# Colors (ANSI, safe, minimal)
G='\033[32m'  # green
Y='\033[33m'  # yellow
R='\033[31m'  # red
B='\033[34m'  # blue
M='\033[35m'  # magenta
C='\033[36m'  # cyan
W='\033[37m'  # white
RESET='\033[0m'


strip_ansi='s/\x1b\[[0-9]*[A-Za-z]//g'

while true; do
    pods=$(sudo kubectl get pods --all-namespaces -o wide)

    Init=$(grep -c Init <<< "$pods")
    Pending=$(grep -c Pending <<< "$pods")
    Running=$(grep -c Running <<< "$pods")
    Unknown=$(grep -c Unknown <<< "$pods")
    Creating=$(grep -c ContainerCreating <<< "$pods")
    Completed=$(grep -c Completed <<< "$pods")

    out=$(cat <<EOF
${B}PODS STATUS ${RESET}$(date '+%H:%M:%S')
${C}Init:${RESET} $Init   ${Y}Pending:${RESET} $Pending   ${G}Running:${RESET} $Running
${M}Unknown:${RESET} $Unknown   ${Y}Creating:${RESET} $Creating   ${W}Completed:${RESET} $Completed

$pods

EOF
)

    clear
    echo -e "$out" \
      | tee >(sed -r "$strip_ansi" > "$LOG_FILE")

    sleep 1
done

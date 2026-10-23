#!/usr/bin/env bash
# Start container k230 and ensure sshd runs inside it (for VS Code Remote-SSH)
# Usage: k230ssh
set -e
CONTAINER=k230

if docker ps --format '{{.Names}}' | grep -qx "$CONTAINER"; then
  echo "[k230ssh] container $CONTAINER already running"
else
  docker start "$CONTAINER" >/dev/null
  echo "[k230ssh] started container $CONTAINER"
fi

if docker exec "$CONTAINER" pgrep -x sshd >/dev/null 2>&1; then
  echo "[k230ssh] sshd already running inside container"
else
  docker exec "$CONTAINER" sudo -n /usr/sbin/sshd
  echo "[k230ssh] started sshd inside container"
fi

IP=$(ip -4 addr show eth0 2>/dev/null | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | head -1)
IP=${IP:-172.27.61.117}
echo "[k230ssh] ready -> ssh user@$IP -p 22222"

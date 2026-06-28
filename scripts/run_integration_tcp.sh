#!/bin/sh
set -u

server="$1"
client="$2"

"$server" &
server_pid=$!

cleanup() {
  if kill -0 "$server_pid" 2>/dev/null; then
    kill -TERM "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

sleep 1
"$client"
client_status=$?
if [ "$client_status" -ne 0 ]; then
  exit "$client_status"
fi

wait "$server_pid"

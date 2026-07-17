#!/bin/sh
set -u

server="$1"
client="$2"
port_file=$(mktemp "${TMPDIR:-/tmp}/redis_simple_tcp_port.XXXXXX")

"$server" "$port_file" &
server_pid=$!

cleanup() {
  if kill -0 "$server_pid" 2>/dev/null; then
    kill -TERM "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
  rm -f "$port_file"
}
trap cleanup EXIT INT TERM

attempt=0
while [ ! -s "$port_file" ] && [ "$attempt" -lt 100 ]; do
  if ! kill -0 "$server_pid" 2>/dev/null; then
    wait "$server_pid"
    exit $?
  fi
  attempt=$((attempt + 1))
  sleep 0.05
done
if [ ! -s "$port_file" ]; then
  exit 1
fi

"$client" "$(cat "$port_file")"
client_status=$?
if [ "$client_status" -ne 0 ]; then
  exit "$client_status"
fi

wait "$server_pid"

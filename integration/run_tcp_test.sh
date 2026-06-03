#!/bin/sh
set -eu

server="$1"
client="$2"

"$server" &
server_pid=$!

cleanup() {
  if kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

sleep 1
"$client"
wait "$server_pid"

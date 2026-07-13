#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="${DOCKER_IMAGE:-ubuntu:24.04}"
container_repo="/work"
container_build_dir="${LINUX_DOCKER_BUILD_DIR:-/tmp/redis_simple_linux_debug}"

docker run --rm \
  --volume "${repo_root}:${container_repo}:ro" \
  --workdir "${container_repo}" \
  "${image}" \
  bash -lc "
set -euo pipefail
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake git
cmake -S ${container_repo} -B ${container_build_dir} \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DREDIS_SIMPLE_WARNINGS_AS_ERRORS=ON
cmake --build ${container_build_dir}
ctest --test-dir ${container_build_dir} -L unit --output-on-failure
ctest --test-dir ${container_build_dir} -L integration --output-on-failure
"

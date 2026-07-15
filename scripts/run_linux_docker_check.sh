#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
base_image="${DOCKER_BASE_IMAGE:-${DOCKER_IMAGE:-ubuntu:24.04}}"
toolchain_image="${DOCKER_TOOLCHAIN_IMAGE:-redis-simple-ubuntu-toolchain:24.04}"
container_repo="/work"
container_build_dir="${LINUX_DOCKER_BUILD_DIR:-/tmp/redis_simple_linux_debug}"

prepare_container=""
cleanup() {
  if [[ -n "${prepare_container}" ]]; then
    docker rm --force "${prepare_container}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if ! docker image inspect "${toolchain_image}" >/dev/null 2>&1; then
  prepare_container="redis-simple-toolchain-${$}"
  docker run --name "${prepare_container}" "${base_image}" bash -lc "
set -euo pipefail
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake git
"
  docker commit "${prepare_container}" "${toolchain_image}" >/dev/null
  docker rm "${prepare_container}" >/dev/null
  prepare_container=""
fi

docker run --rm \
  --network none \
  --cap-drop ALL \
  --security-opt no-new-privileges \
  --volume "${repo_root}:${container_repo}:ro" \
  --tmpfs /tmp:rw,exec,nosuid,size=2g \
  --workdir "${container_repo}" \
  "${toolchain_image}" \
  bash -lc "
set -euo pipefail
cmake -S ${container_repo} -B ${container_build_dir} \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DREDIS_SIMPLE_WARNINGS_AS_ERRORS=ON
cmake --build ${container_build_dir}
ctest --test-dir ${container_build_dir} -L unit --output-on-failure
ctest --test-dir ${container_build_dir} -L integration --output-on-failure
"

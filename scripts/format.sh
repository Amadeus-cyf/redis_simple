#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

llvm_bin_dirs=(
  "/opt/homebrew/opt/llvm/bin"
  "/usr/local/opt/llvm/bin"
)

if command -v brew >/dev/null 2>&1; then
  llvm_prefix="$(brew --prefix llvm 2>/dev/null || true)"
  if [[ -n "${llvm_prefix}" ]]; then
    llvm_bin_dirs=("${llvm_prefix}/bin" "${llvm_bin_dirs[@]}")
  fi
fi

find_tool() {
  local tool_name="$1"
  local override="${2:-}"

  if [[ -n "${override}" ]]; then
    command -v "${override}" || return 1
    return 0
  fi

  if command -v "${tool_name}" >/dev/null 2>&1; then
    command -v "${tool_name}"
    return 0
  fi

  local bin_dir
  for bin_dir in "${llvm_bin_dirs[@]}"; do
    if [[ -x "${bin_dir}/${tool_name}" ]]; then
      printf '%s\n' "${bin_dir}/${tool_name}"
      return 0
    fi
  done

  return 1
}

clang_format_bin="$(find_tool clang-format "${CLANG_FORMAT_BIN:-}")"

cd "${repo_root}"
files=()
while IFS= read -r file; do
  if [[ -f "${file}" ]]; then
    files+=("${file}")
  fi
done < <(git ls-files '*.h' '*.cpp' ':!:third_party/**' ':!:build/**')

if (( ${#files[@]} == 0 )); then
  exit 0
fi

"${clang_format_bin}" -i "${files[@]}"

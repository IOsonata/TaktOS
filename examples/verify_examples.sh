#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INCLUDE_FLAGS=(-I"${ROOT_DIR}/include" -I"${ROOT_DIR}/ARM/include")

CC_CMD="${CC:-clang}"
CXX_CMD="${CXX:-clang++}"
CFLAGS=("${CFLAGS:-}" -std=gnu11 -fsyntax-only)
CXXFLAGS=("${CXXFLAGS:-}" -std=gnu++17 -fsyntax-only)

C_EXAMPLES=(
  "${ROOT_DIR}/examples/basic/basic_c.c"
  "${ROOT_DIR}/examples/queue/producer_consumer_c.c"
  "${ROOT_DIR}/examples/mutex/mutex_pi_c.c"
  "${ROOT_DIR}/examples/posix/posix_pse51.c"
  "${ROOT_DIR}/examples/mpu_guard.c"
)

CPP_EXAMPLES=(
  "${ROOT_DIR}/examples/basic/basic_cpp.cpp"
  "${ROOT_DIR}/examples/queue/producer_consumer.cpp"
  "${ROOT_DIR}/examples/mutex/mutex_pi.cpp"
  "${ROOT_DIR}/examples/posix/posix_pse51_cpp.cpp"
)

run_checks() {
  local compiler="$1"
  shift
  local -a args=("$@")
  local file

  if ! command -v "$compiler" >/dev/null 2>&1; then
    echo "error: compiler '$compiler' not found" >&2
    return 127
  fi

  for file in "${args[@]}"; do
    echo "checking ${file#${ROOT_DIR}/}"
    "$compiler" "${INCLUDE_FLAGS[@]}" "${@: -0}" >/dev/null 2>&1
  done
}

for file in "${C_EXAMPLES[@]}"; do
  echo "checking ${file#${ROOT_DIR}/}"
  "$CC_CMD" -std=gnu11 -fsyntax-only "${INCLUDE_FLAGS[@]}" "$file"
done

for file in "${CPP_EXAMPLES[@]}"; do
  echo "checking ${file#${ROOT_DIR}/}"
  "$CXX_CMD" -std=gnu++17 -fsyntax-only "${INCLUDE_FLAGS[@]}" "$file"
done

echo "all examples passed syntax verification"

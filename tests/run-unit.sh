#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/bin/tests/unit"
CC_BIN="${CC:-cc}"
CFLAGS=(
    -std=c11
    -Wall
    -Wextra
    -Werror
    -I"${ROOT}/include"
    -I"${ROOT}/tests/unit"
)

mkdir -p "${BUILD_DIR}"

run_test() {
    local source_file="$1"
    local output_file

    output_file="${BUILD_DIR}/$(basename "${source_file%.c}")"

    "${CC_BIN}" \
        "${CFLAGS[@]}" \
        "${ROOT}/src/input/input_ring.c" \
        "${ROOT}/src/input/input.c" \
        "${ROOT}/src/tty/tty.c" \
        "${ROOT}/src/console/display.c" \
        "${ROOT}/src/console/text_console.c" \
        "${ROOT}/src/kernel/bootstrap_paging.c" \
        "${ROOT}/src/kernel/process.c" \
        "${ROOT}/src/kernel/io.c" \
        "${ROOT}/src/kernel/syscall.c" \
        "${ROOT}/tests/unit/test.c" \
        "${source_file}" \
        -o "${output_file}"

    "${output_file}"
}

status=0

for test_source in "${ROOT}"/tests/unit/test_*.c; do
    if ! run_test "${test_source}"; then
        status=1
    fi
done

exit "${status}"

#!/usr/bin/env bash

set -euo pipefail

RUN_QEMU="${RUN_QEMU:-1}"

if [[ $# -gt 1 ]]; then
    printf 'Usage: %s [--build-only]\n' "$0" >&2
    exit 1
fi

if [[ $# -eq 1 ]]; then
    case "$1" in
        --build-only)
            RUN_QEMU=0
            ;;
        *)
            printf 'Usage: %s [--build-only]\n' "$0" >&2
            exit 1
            ;;
    esac
fi

find_tool() {
    local tool

    for tool in "$@"; do
        if [[ -z "${tool}" ]]; then
            continue
        fi

        if [[ "${tool}" == */* ]] && [[ -x "${tool}" ]]; then
            printf '%s\n' "${tool}"
            return 0
        fi

        if command -v "${tool}" >/dev/null 2>&1; then
            command -v "${tool}"
            return 0
        fi
    done

    return 1
}

require_tool() {
    local name="$1"
    local install_hint="$2"
    shift 2

    if ! find_tool "$@" >/dev/null 2>&1; then
        printf 'Missing %s.\n' "${name}" >&2
        printf '%s\n' "${install_hint}" >&2
        exit 1
    fi
}

OS="$(uname -s)"
BREW_PREFIX=""

if command -v brew >/dev/null 2>&1; then
    BREW_PREFIX="$(brew --prefix 2>/dev/null || true)"
fi

NASM="$(find_tool nasm || true)"
TRUNCATE_BIN="$(find_tool truncate gtruncate || true)"

if [[ "${OS}" == "Darwin" ]]; then
    CC="$(find_tool i686-elf-gcc x86_64-elf-gcc "${BREW_PREFIX}/opt/llvm/bin/clang" clang || true)"
    LD_BIN="$(find_tool i686-elf-ld x86_64-elf-ld ld.lld lld "${BREW_PREFIX}/opt/lld/bin/ld.lld" "${BREW_PREFIX}/opt/lld/bin/lld" || true)"
    OBJCOPY_BIN="$(find_tool i686-elf-objcopy x86_64-elf-objcopy llvm-objcopy objcopy gobjcopy "${BREW_PREFIX}/opt/llvm/bin/llvm-objcopy" "${BREW_PREFIX}/opt/binutils/bin/objcopy" "${BREW_PREFIX}/opt/binutils/bin/gobjcopy" || true)"
    QEMU_BIN="$(find_tool qemu-system-i386 "${BREW_PREFIX}/bin/qemu-system-i386" || true)"

    require_tool \
        "an ELF linker" \
        "On macOS install LLVM lld, e.g. \`brew install lld\`." \
        "${LD_BIN:-}" i686-elf-ld x86_64-elf-ld ld.lld lld "${BREW_PREFIX}/opt/lld/bin/ld.lld" "${BREW_PREFIX}/opt/lld/bin/lld"
    require_tool \
        "objcopy" \
        "On macOS install LLVM or GNU binutils, e.g. \`brew install llvm\` or \`brew install binutils\`." \
        "${OBJCOPY_BIN:-}" i686-elf-objcopy x86_64-elf-objcopy llvm-objcopy objcopy gobjcopy "${BREW_PREFIX}/opt/llvm/bin/llvm-objcopy" "${BREW_PREFIX}/opt/binutils/bin/objcopy" "${BREW_PREFIX}/opt/binutils/bin/gobjcopy"
    require_tool \
        "QEMU" \
        "Install QEMU, e.g. \`brew install qemu\`." \
        "${QEMU_BIN:-}" qemu-system-i386 "${BREW_PREFIX}/bin/qemu-system-i386"

    CFLAGS=(
        -target i386-none-elf
        -m32
        -ffreestanding
        -fno-pic
        -fno-pie
        -fno-stack-protector
        -nostdlib
        -Wall
        -Wextra
        -Werror=implicit-function-declaration
        -c
    )
else
    CC="$(find_tool i686-elf-gcc x86_64-elf-gcc gcc clang || true)"
    LD_BIN="$(find_tool i686-elf-ld x86_64-elf-ld ld || true)"
    OBJCOPY_BIN="$(find_tool i686-elf-objcopy x86_64-elf-objcopy objcopy llvm-objcopy || true)"
    QEMU_BIN="$(find_tool qemu-system-i386 || true)"

    CFLAGS=(
        -m32
        -ffreestanding
        -fno-pic
        -fno-pie
        -fno-stack-protector
        -nostdlib
        -nostartfiles
        -Wall
        -Wextra
        -Werror=implicit-function-declaration
        -c
    )
fi

require_tool \
    "nasm" \
    "Install NASM and re-run the build." \
    "${NASM:-}" nasm
require_tool \
    "truncate" \
    "Install GNU coreutils if your system does not provide \`truncate\`." \
    "${TRUNCATE_BIN:-}" truncate gtruncate
require_tool \
    "a C compiler" \
    "Install a compiler that can emit 32-bit freestanding code." \
    "${CC:-}" i686-elf-gcc x86_64-elf-gcc gcc clang
require_tool \
    "a linker" \
    "Install a linker that supports ELF i386 output." \
    "${LD_BIN:-}" i686-elf-ld x86_64-elf-ld ld gld
require_tool \
    "objcopy" \
    "Install GNU binutils or LLVM objcopy." \
    "${OBJCOPY_BIN:-}" i686-elf-objcopy x86_64-elf-objcopy objcopy llvm-objcopy gobjcopy
require_tool \
    "QEMU" \
    "Install QEMU and make sure \`qemu-system-i386\` is on PATH." \
    "${QEMU_BIN:-}" qemu-system-i386

BUILD_DIR="bin"
BOOT_BIN="${BUILD_DIR}/bootS.bin"
KERNEL_OBJ="${BUILD_DIR}/kernel.o"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
DISK_IMAGE="${BUILD_DIR}/disk.img"
DISK_SIZE="10M"

mkdir -p "${BUILD_DIR}"

"${NASM}" -f bin bootS.asm -o "${BOOT_BIN}"
"${TRUNCATE_BIN}" -s "${DISK_SIZE}" "${DISK_IMAGE}"
dd if="${BOOT_BIN}" of="${DISK_IMAGE}" bs=512 conv=notrunc

"${CC}" "${CFLAGS[@]}" kernel.c -o "${KERNEL_OBJ}"
"${LD_BIN}" -m elf_i386 -T link.ld -o "${KERNEL_ELF}" "${KERNEL_OBJ}"
"${OBJCOPY_BIN}" -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"
"${TRUNCATE_BIN}" -s 1048576 "${KERNEL_BIN}"

dd if="${KERNEL_BIN}" of="${DISK_IMAGE}" bs=512 seek=64 conv=notrunc

if [[ "${RUN_QEMU}" == "1" ]]; then
    "${QEMU_BIN}" -m 512 -drive file="${DISK_IMAGE}",format=raw
fi

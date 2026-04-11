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
BUILD_OBJ_DIR="${BUILD_DIR}/obj"
USER_BUILD_DIR="${BUILD_DIR}/user"
USER_BUILD_OBJ_DIR="${BUILD_OBJ_DIR}/user"
KERNEL_OBJ="${BUILD_OBJ_DIR}/kernel.o"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
DISK_IMAGE="${BUILD_DIR}/disk.img"
DISK_SIZE="10M"
INCLUDE_FLAGS=(
    -Iinclude
)

mkdir -p "${BUILD_DIR}" "${BUILD_OBJ_DIR}" "${USER_BUILD_DIR}" "${USER_BUILD_OBJ_DIR}"

"${NASM}" -f bin bootS.asm -o "${BOOT_BIN}"
"${TRUNCATE_BIN}" -s "${DISK_SIZE}" "${DISK_IMAGE}"
dd if="${BOOT_BIN}" of="${DISK_IMAGE}" bs=512 conv=notrunc

"${CC}" "${INCLUDE_FLAGS[@]}" "${CFLAGS[@]}" kernel.c -o "${KERNEL_OBJ}"

KERNEL_OBJECTS=("${KERNEL_OBJ}")
USER_OBJECTS=()

compile_source_tree() {
    local source_root="$1"
    local build_root="$2"
    local array_name="$3"
    local source_file=""
    local object_file=""

    if [[ ! -d "${source_root}" ]]; then
        return 0
    fi

    while IFS= read -r -d '' source_file; do
        object_file="${build_root}/${source_file}.o"
        mkdir -p "$(dirname "${object_file}")"
        "${CC}" "${INCLUDE_FLAGS[@]}" "${CFLAGS[@]}" "${source_file}" -o "${object_file}"
        eval "${array_name}+=(\"\${object_file}\")"
    done < <(find "${source_root}" -type f \( -name '*.c' -o -name '*.S' \) -print0)
}

compile_source_tree src "${BUILD_OBJ_DIR}" KERNEL_OBJECTS

build_user_program() {
    local app_name="$1"
    local app_source="$2"
    local user_elf="${USER_BUILD_DIR}/${app_name}.elf"
    local user_blob_obj="${USER_BUILD_OBJ_DIR}/${app_name}_elf_blob.o"
    local user_objects=()

    while IFS= read -r -d '' source_file; do
        object_file="${USER_BUILD_OBJ_DIR}/${source_file}.o"
        mkdir -p "$(dirname "${object_file}")"
        "${CC}" "${INCLUDE_FLAGS[@]}" "${CFLAGS[@]}" "${source_file}" -o "${object_file}"
        user_objects+=("${object_file}")
    done < <(find user/lib -type f \( -name '*.c' -o -name '*.S' \) -print0)

    object_file="${USER_BUILD_OBJ_DIR}/${app_source}.o"
    mkdir -p "$(dirname "${object_file}")"
    "${CC}" "${INCLUDE_FLAGS[@]}" "${CFLAGS[@]}" "${app_source}" -o "${object_file}"
    user_objects+=("${object_file}")

    "${LD_BIN}" -m elf_i386 -T user/link.ld -o "${user_elf}" "${user_objects[@]}"

    "${OBJCOPY_BIN}" \
        -I binary \
        -O elf32-i386 \
        -B i386 \
        --rename-section .data=.rodata.user_image,alloc,load,readonly,data,contents \
        --redefine-sym _binary_bin_user_${app_name}_elf_start=_user_${app_name}_elf_start \
        --redefine-sym _binary_bin_user_${app_name}_elf_end=_user_${app_name}_elf_end \
        "${user_elf}" \
        "${user_blob_obj}"

    KERNEL_OBJECTS+=("${user_blob_obj}")
}

build_user_program echo_line user/apps/echo_line.c

"${LD_BIN}" -m elf_i386 -T link.ld -o "${KERNEL_ELF}" "${KERNEL_OBJECTS[@]}"
"${OBJCOPY_BIN}" -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"
"${TRUNCATE_BIN}" -s 1048576 "${KERNEL_BIN}"

dd if="${KERNEL_BIN}" of="${DISK_IMAGE}" bs=512 seek=64 conv=notrunc

if [[ "${RUN_QEMU}" == "1" ]]; then
    "${QEMU_BIN}" -m 512 -drive file="${DISK_IMAGE}",format=raw
fi

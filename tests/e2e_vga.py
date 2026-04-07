#!/usr/bin/env python3

import re
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
EXPECTED_TEXT = "Hello, World!"
QEMU_TIMEOUT_SECONDS = 5
BOOT_WAIT_SECONDS = 1.0
VGA_DUMP_BYTES = 160


def build_image() -> None:
    subprocess.run(
        [str(ROOT / "build.sh"), "--build-only"],
        cwd=ROOT,
        check=True,
    )


def decode_vga_text(monitor_output: str) -> str:
    dump_lines = [
        line
        for line in monitor_output.splitlines()
        if re.match(r"^[0-9a-fA-F]+:\s", line)
    ]
    bytes_found = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", "\n".join(dump_lines))]
    chars = []

    for i in range(0, len(bytes_found), 2):
        if i + 1 >= len(bytes_found):
            break
        chars.append(chr(bytes_found[i]))

    return "".join(chars).rstrip()


def read_vga_text() -> str:
    disk_image = ROOT / "bin" / "disk.img"
    qemu_cmd = [
        "qemu-system-i386",
        "-display",
        "none",
        "-monitor",
        "stdio",
        "-serial",
        "none",
        "-m",
        "512",
        "-drive",
        f"file={disk_image},format=raw",
    ]

    proc = subprocess.Popen(
        qemu_cmd,
        cwd=ROOT,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    try:
        time.sleep(BOOT_WAIT_SECONDS)
        monitor_cmd = f"xp /{VGA_DUMP_BYTES}bx 0xb8000\nquit\n"
        output, _ = proc.communicate(monitor_cmd, timeout=QEMU_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        proc.kill()
        output, _ = proc.communicate()
        raise RuntimeError(f"QEMU monitor timed out.\n{output}") from None

    if proc.returncode not in (0, None):
        raise RuntimeError(f"QEMU exited with code {proc.returncode}.\n{output}")

    return decode_vga_text(output)


def main() -> int:
    build_image()
    vga_text = read_vga_text()

    if EXPECTED_TEXT not in vga_text:
        print("VGA assertion failed.", file=sys.stderr)
        print(f"Expected to find: {EXPECTED_TEXT!r}", file=sys.stderr)
        print(f"Actual VGA text:   {vga_text!r}", file=sys.stderr)
        return 1

    print(f"VGA assertion passed: found {EXPECTED_TEXT!r}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

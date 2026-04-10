#!/usr/bin/env python3

import re
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
EXPECTED_BOOT_TEXT = "interrupts ready"
QEMU_TIMEOUT_SECONDS = 5
BOOT_WAIT_SECONDS = 1.0
KEY_WAIT_SECONDS = 0.5
VGA_DUMP_BYTES = 80 * 2 * 3


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
    bytes_found = [
        int(value, 16)
        for value in re.findall(r"0x([0-9a-fA-F]{2})", "\n".join(dump_lines))
    ]
    chars = []

    for i in range(0, len(bytes_found), 2):
        if i + 1 >= len(bytes_found):
            break
        chars.append(chr(bytes_found[i]))

    return "".join(chars).rstrip()


def read_vga_text_after_keypress() -> str:
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
        proc.stdin.write("sendkey a\n")
        proc.stdin.flush()
        time.sleep(KEY_WAIT_SECONDS)
        proc.stdin.write(f"xp /{VGA_DUMP_BYTES}bx 0xb8000\n")
        proc.stdin.write("quit\n")
        proc.stdin.flush()
        output, _ = proc.communicate(timeout=QEMU_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        proc.kill()
        output, _ = proc.communicate()
        raise RuntimeError(f"QEMU monitor timed out.\n{output}") from None

    if proc.returncode not in (0, None):
        raise RuntimeError(f"QEMU exited with code {proc.returncode}.\n{output}")

    return decode_vga_text(output)


def main() -> int:
    build_image()
    vga_text = read_vga_text_after_keypress()

    if EXPECTED_BOOT_TEXT not in vga_text:
        print("Keyboard IRQ assertion [FAILED].", file=sys.stderr)
        print(f"Missing boot text:  {EXPECTED_BOOT_TEXT!r}", file=sys.stderr)
        print(f"Actual VGA text:    {vga_text!r}", file=sys.stderr)
        return 1

    boot_and_tail = vga_text.split(EXPECTED_BOOT_TEXT, 1)[1]

    if "a" not in boot_and_tail:
        print("Keyboard IRQ assertion [FAILED].", file=sys.stderr)
        print("Missing visible key effect after boot text.", file=sys.stderr)
        print(f"Actual VGA text:    {vga_text!r}", file=sys.stderr)
        return 1

    print(
        "Keyboard IRQ assertion [PASSED]: "
        f"found boot text and visible key echo (Actual VGA text: {vga_text!r})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

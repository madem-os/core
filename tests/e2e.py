#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
QEMU_TIMEOUT_SECONDS = 5
BOOT_WAIT_SECONDS = 1.0
KEY_WAIT_SECONDS = 0.5
VGA_DUMP_BYTES = 80 * 2 * 15
PROMPT = b"(qemu) "


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


class QemuSession:
    def __init__(self) -> None:
        self.proc = None

    def start(self) -> None:
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

        self.proc = subprocess.Popen(
            qemu_cmd,
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=False,
        )

        self._read_until_prompt()
        time.sleep(BOOT_WAIT_SECONDS)

    def stop(self) -> None:
        if self.proc is None:
            return

        try:
            self.command("quit")
        except Exception:
            self.proc.kill()
            self.proc.wait(timeout=QEMU_TIMEOUT_SECONDS)

    def command(self, text: str) -> str:
        if self.proc is None or self.proc.stdin is None:
            raise RuntimeError("QEMU session is not running.")

        self.proc.stdin.write((text + "\n").encode("ascii"))
        self.proc.stdin.flush()
        return self._read_until_prompt().decode("utf-8", errors="replace")

    def dump_vga_text(self) -> str:
        return decode_vga_text(self.command(f"xp /{VGA_DUMP_BYTES}bx 0xb8000"))

    def send_key(self, key: str) -> None:
        self.command(f"sendkey {key}")
        time.sleep(KEY_WAIT_SECONDS)

    def _read_until_prompt(self) -> bytes:
        if self.proc is None or self.proc.stdout is None:
            raise RuntimeError("QEMU session is not running.")

        deadline = time.time() + QEMU_TIMEOUT_SECONDS
        chunks = bytearray()

        while time.time() < deadline:
            byte = self.proc.stdout.read(1)

            if not byte:
                if self.proc.poll() is not None:
                    raise RuntimeError(
                        f"QEMU exited with code {self.proc.returncode}.\n"
                        f"{chunks.decode('utf-8', errors='replace')}"
                    )
                continue

            chunks.extend(byte)
            if chunks.endswith(PROMPT):
                return bytes(chunks)

        raise RuntimeError(
            "Timed out waiting for QEMU monitor prompt.\n"
            f"{chunks.decode('utf-8', errors='replace')}"
        )


def test_boot_text(session: QemuSession) -> None:
    vga_text = session.dump_vga_text()

    if "welcome to Madem-OS!" not in vga_text:
        raise AssertionError(
            "Missing expected boot banner.\n"
            f"Actual VGA text: {vga_text!r}"
        )

    if "kernel_entry=0xC0100000" not in vga_text:
        raise AssertionError(
            "Missing expected higher-half kernel address print.\n"
            f"Actual VGA text: {vga_text!r}"
        )

    if "user mode ready" not in vga_text:
        raise AssertionError(
            "Missing expected user mode startup text.\n"
            f"Actual VGA text: {vga_text!r}"
        )

def test_keyboard_input(session: QemuSession) -> None:
    before = session.dump_vga_text()
    session.send_key("q")
    after = session.dump_vga_text()

    if "q" not in after:
        raise AssertionError(
            "Missing visible keyboard input effect after sendkey.\n"
            f"Before: {before!r}\n"
            f"After:  {after!r}"
        )
    session.send_key("ret")    

def test_user_page_fault(session: QemuSession) -> None:
    session.send_key("shift-4")
    session.send_key("ret")
    after = session.dump_vga_text()

    if "page fault:" not in after:
        raise AssertionError(
            "Missing expected page fault panic after forbidden user write.\n"
            f"Actual VGA text: {after!r}"
        )


TEST_CASES = {
    "boot_text": test_boot_text,
    "keyboard_input": test_keyboard_input,
    "user_page_fault": test_user_page_fault,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run MademOS e2e tests in one QEMU session.")
    parser.add_argument(
        "tests",
        nargs="*",
        help="Optional subset of test names to run.",
    )
    args = parser.parse_args()

    invalid = [name for name in args.tests if name not in TEST_CASES]
    if invalid:
        parser.error(
            "unknown test name(s): "
            + ", ".join(invalid)
            + ". valid choices: "
            + ", ".join(sorted(TEST_CASES.keys()))
        )

    return args


def main() -> int:
    args = parse_args()
    selected_tests = args.tests or list(TEST_CASES.keys())

    build_image()
    session = QemuSession()
    session.start()

    try:
        for test_name in selected_tests:
            TEST_CASES[test_name](session)
            print(f"[PASSED] {test_name}")
    except AssertionError as exc:
        print(f"[FAILED] {test_name}", file=sys.stderr)
        print(str(exc), file=sys.stderr)
        return 1
    finally:
        session.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

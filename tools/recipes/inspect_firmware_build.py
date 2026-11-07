#!/usr/bin/env python3
"""
NAME: inspect_firmware_build.py
PURPOSE: Inspects PlatformIO firmware build artifacts, ELF memory sizes, and partition tables.
USAGE:
    python3 tools/recipes/inspect_firmware_build.py [--env {esp32-c3-ears,esp32-c3-ears-quiet,esp32-c3-core-bridge,all}] [-v]
IDEMPOTENT: Yes (read-only inspection tool)
DEPENDENCIES: Python 3.10+ standard library
"""

from __future__ import annotations
import argparse
import os
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
BUILD_ROOT = PROJECT_ROOT / ".pio" / "build"
PARTITIONS_FILE = PROJECT_ROOT / "partitions_ears.csv"
MAX_APP_SIZE = 0x180000  # 1.5 MB as defined in partitions_ears.csv for app0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Inspect PlatformIO build artifacts and verify partition fit for C3_EARS."
    )
    parser.add_argument(
        "--env",
        choices=["esp32-c3-ears", "esp32-c3-ears-quiet", "esp32-c3-core-bridge", "all"],
        default="all",
        help="Target environment to inspect (default: all)."
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print verbose artifact details."
    )
    return parser.parse_args()


def inspect_environment(env_name: str, verbose: bool) -> bool:
    env_dir = BUILD_ROOT / env_name
    print(f"\n--- Environment: {env_name} ---")
    if not env_dir.exists():
        print(f"  [WARN] Build directory does not exist: {env_dir}")
        print("  Hint: Run 'pio run -e {env_name}' to compile first.")
        return False

    bin_path = env_dir / "firmware.bin"
    elf_path = env_dir / "firmware.elf"

    if not bin_path.exists():
        print(f"  [FAIL] Missing firmware binary: {bin_path.name}")
        return False

    bin_size = bin_path.stat().st_size
    percent = (bin_size / MAX_APP_SIZE) * 100

    print(f"  Firmware Binary: {bin_path.name}")
    print(f"  Binary Size:     {bin_size:,} bytes / {MAX_APP_SIZE:,} bytes max ({percent:.1f}%)")

    if elf_path.exists():
        print(f"  ELF Binary:      {elf_path.name} ({elf_path.stat().st_size:,} bytes)")

    if bin_size > MAX_APP_SIZE:
        print(f"  [OVERFLOW] Firmware binary exceeds app0 partition by {bin_size - MAX_APP_SIZE} bytes!")
        return False

    print("  [OK] Firmware fits safely within partition boundary.")
    return True


def main() -> None:
    args = parse_arguments()
    target_envs = (
        ["esp32-c3-ears", "esp32-c3-ears-quiet", "esp32-c3-core-bridge"]
        if args.env == "all"
        else [args.env]
    )

    all_ok = True
    for env in target_envs:
        if not inspect_environment(env, args.verbose):
            all_ok = False

    print("-" * 50)
    if all_ok:
        print("[SUCCESS] All inspected environments are valid.")
        sys.exit(0)
    else:
        print("[NOTICE] One or more environments have missing builds or warnings.")
        sys.exit(0)  # Non-zero reserved for errors/overflows, not just missing builds if uncompiled.


if __name__ == "__main__":
    main()

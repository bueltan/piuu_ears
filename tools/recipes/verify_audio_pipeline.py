#!/usr/bin/env python3
"""
NAME: verify_audio_pipeline.py
PURPOSE: Automated runner and validator for audio routing and capture unit tests.
USAGE:
    python3 tools/recipes/verify_audio_pipeline.py [--suite {all,routing,diagnostics}] [--verbose]
IDEMPOTENT: Yes (read-only test runner and status reporter)
DEPENDENCIES: Python 3.10+ standard library, g++, bash
"""

from __future__ import annotations
import argparse
import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run C3_EARS audio routing and signal processing unit tests."
    )
    parser.add_argument(
        "--suite",
        choices=["all", "routing", "diagnostics"],
        default="all",
        help="Test suite subset to run (default: all)."
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print verbose command outputs."
    )
    return parser.parse_args()


def run_test_script(script_path: Path, verbose: bool) -> bool:
    print(f"[TEST] Executing {script_path.name}...")
    result = subprocess.run(
        ["bash", str(script_path)],
        cwd=PROJECT_ROOT,
        capture_output=not verbose,
        text=True
    )
    if result.returncode == 0:
        print(f"[PASS] {script_path.name} completed successfully.")
        return True
    else:
        print(f"[FAIL] {script_path.name} failed with exit code {result.returncode}.")
        if not verbose and result.stderr:
            print(result.stderr.strip())
        return False


def main() -> None:
    args = parse_arguments()
    scripts: list[Path] = []

    if args.suite in ("all", "routing"):
        scripts.append(PROJECT_ROOT / "test" / "run_audio_routing_tests.sh")
    if args.suite in ("all", "diagnostics"):
        scripts.append(PROJECT_ROOT / "test" / "run_diagnostics_tests.sh")

    all_passed = True
    for script in scripts:
        if not script.exists():
            print(f"[ERROR] Test script not found: {script}")
            sys.exit(1)
        if not run_test_script(script, args.verbose):
            all_passed = False

    if all_passed:
        print("\n[SUCCESS] Audio pipeline and signal processing tests passed.")
        sys.exit(0)
    else:
        print("\n[FAILURE] One or more test suites failed.")
        sys.exit(1)


if __name__ == "__main__":
    main()

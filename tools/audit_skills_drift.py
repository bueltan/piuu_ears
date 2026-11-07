#!/usr/bin/env python3
"""
NAME: audit_skills_drift.py
PURPOSE: Automated validator to detect documentation drift, broken markdown links, missing script references, and recipe compliance across C3_EARS.
USAGE:
    python3 tools/audit_skills_drift.py [--verbose]
IDEMPOTENT: Yes (read-only verification tool)
"""

from __future__ import annotations
import os
import re
import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent

CORE_DOCS = [
    PROJECT_ROOT / "AGENTS.md",
    PROJECT_ROOT / "SKILLS.md",
    PROJECT_ROOT / "STATE.md",
]

SKILL_DIR = PROJECT_ROOT / "skills"
RECIPES_DIR = PROJECT_ROOT / "tools" / "recipes"


def check_file_links(doc_path: Path, verbose: bool = False) -> list[str]:
    """Finds and validates all markdown links in a file."""
    issues: list[str] = []
    if not doc_path.exists():
        return [f"File missing: {doc_path}"]

    content = doc_path.read_text(encoding="utf-8")
    links = re.findall(r"\[([^\]]+)\]\(([^)]+)\)", content)

    for label, target in links:
        # Ignore external links, mailto, and pure anchor tags
        if target.startswith("http://") or target.startswith("https://") or target.startswith("#") or target.startswith("mailto:"):
            continue

        # Strip internal page anchors (e.g. file.md#section -> file.md)
        clean_target = target.split("#")[0]
        if not clean_target:
            continue

        if clean_target.startswith("file://"):
            resolved = Path(clean_target.replace("file://", "")).resolve()
        else:
            resolved = (doc_path.parent / clean_target).resolve()

        if not resolved.exists():
            issues.append(f"Broken link in '{doc_path.name}': [{label}]({target}) -> '{resolved}' does not exist.")
        elif verbose:
            print(f"  [OK link] {doc_path.name}: {target} -> {resolved.relative_to(PROJECT_ROOT) if PROJECT_ROOT in resolved.parents else resolved}")

    return issues


def check_script_references(doc_path: Path, verbose: bool = False) -> list[str]:
    """Scans code blocks and text for references to tools/*.py or test/*.sh and ensures they exist."""
    issues: list[str] = []
    content = doc_path.read_text(encoding="utf-8")

    # Match patterns like tools/<name>.py, test/<name>.sh, or relative ../<project>/tools/<name>.py
    script_refs = re.findall(r"(?:(?:\.\./[a-zA-Z0-9_\-]+/)?(?:tools|test)/[a-zA-Z0-9_\-/\.]+\.(?:py|sh|cpp))", content)
    for ref in set(script_refs):
        # Ignore placeholders
        if "<" in ref or ">" in ref or "recipe_name" in ref:
            continue
        resolved = (PROJECT_ROOT / ref).resolve()
        if not resolved.exists():
            issues.append(f"Obsolete/Missing script reference in '{doc_path.name}': '{ref}' does not exist on disk.")
        elif verbose:
            print(f"  [OK script] {doc_path.name}: {ref}")

    return issues


def scan_recipes(verbose: bool = False) -> list[str]:
    """Lists available agent recipes, validates their docstring header and verifies --help CLI support."""
    issues: list[str] = []
    if not RECIPES_DIR.exists():
        return issues

    recipes = [p for p in RECIPES_DIR.glob("*.py") if p.name != "__init__.py"]
    for recipe in recipes:
        text = recipe.read_text(encoding="utf-8")
        required_keys = ["NAME:", "PURPOSE:", "USAGE:", "IDEMPOTENT:"]
        for key in required_keys:
            if key not in text:
                issues.append(f"Recipe '{recipe.name}' missing mandatory docstring key: '{key}'.")

        # Verify CLI --help support in 3-line context economy standard
        try:
            result = subprocess.run(
                [sys.executable, str(recipe), "--help"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=5,
            )
            if result.returncode != 0:
                issues.append(f"Recipe '{recipe.name}' --help returned non-zero exit code: {result.returncode}")
            elif verbose:
                print(f"  [OK recipe CLI] {recipe.name} passed --help inspection.")
        except Exception as e:
            issues.append(f"Recipe '{recipe.name}' failed to execute with --help: {e}")

        if verbose and not issues:
            print(f"  [OK recipe] {recipe.name}")

    return issues


def main() -> None:
    verbose = "-v" in sys.argv or "--verbose" in sys.argv
    all_issues: list[str] = []

    print("[AUDIT] Starting Skills & Documentation Drift Audit for C3_EARS...")

    # 1. Audit core docs links & scripts
    all_docs = list(CORE_DOCS) + list(SKILL_DIR.glob("*.md"))
    recipe_readme = RECIPES_DIR / "README.md"
    if recipe_readme.exists():
        all_docs.append(recipe_readme)

    for doc in all_docs:
        link_issues = check_file_links(doc, verbose=verbose)
        all_issues.extend(link_issues)

        script_issues = check_script_references(doc, verbose=verbose)
        all_issues.extend(script_issues)

    # 2. Check recipes
    recipe_issues = scan_recipes(verbose=verbose)
    all_issues.extend(recipe_issues)

    # Summary
    print("-" * 65)
    if all_issues:
        print(f"[FAIL] Detected {len(all_issues)} drift issue(s):")
        for idx, issue in enumerate(all_issues, 1):
            print(f"  {idx}. {issue}")
        print("\nAction Required: Correct broken links or update manifests before concluding session.")
        sys.exit(1)
    else:
        num_docs = len(all_docs)
        recipe_count = len([p for p in RECIPES_DIR.glob("*.py") if p.name != "__init__.py"])
        print(f"[PASS] 0 drift issues found across {num_docs} documentation manifests and skill guides.")
        print(f"[INFO] Active agent recipes in tools/recipes/: {recipe_count}")
        sys.exit(0)


if __name__ == "__main__":
    main()

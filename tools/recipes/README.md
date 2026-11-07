# Reusable Agent Recipes Repository (`tools/recipes/`)

> **Protocol Notice**: This directory houses idempotent, self-documenting automation recipes and maintenance scripts created by AI agents and developers. It follows the PIUU Antigravity governance standard defined in [`AGENTS.md`](../../AGENTS.md).

---

## 🎯 Architecture & Context Economy Rules

Incoming AI agents must **never** dump complete recipe script source code into the active chat context. Instead:

1. **3-Line Zero-Context Inspection**: Every recipe must support standard `argparse` `--help`:
   ```bash
   python3 tools/recipes/<recipe_name>.py --help
   ```
2. **Standard Docstring Header**: Every recipe must begin with the standard 4-key metadata header:
   ```python
   """
   NAME: <script_name>.py
   PURPOSE: <one-line summary of functionality>
   USAGE: python3 tools/recipes/<script_name>.py [--flags]
   IDEMPOTENT: Yes | No
   """
   ```
3. **Guaranteed Idempotence**: Running a recipe multiple times with identical arguments must produce the exact same outcome without creating duplicate configurations, orphaned files, or corrupted buffers.
4. **Exit Codes**: Must return `0` on success and non-zero on failure.
5. **Auditing**: Verified automatically by `python3 tools/audit_skills_drift.py`.

---

## 📁 Available Recipes

| Recipe | Description | Usage |
| :--- | :--- | :--- |
| [`template_recipe.py`](template_recipe.py) | Boilerplate starter template for new agent recipes. | `python3 tools/recipes/template_recipe.py --help` |
| [`verify_audio_pipeline.py`](verify_audio_pipeline.py) | Automated runner and validator for audio routing and capture unit tests. | `python3 tools/recipes/verify_audio_pipeline.py --help` |
| [`inspect_firmware_build.py`](inspect_firmware_build.py) | Inspects PlatformIO firmware build artifacts, ELF memory sizes, and partition tables. | `python3 tools/recipes/inspect_firmware_build.py --help` |

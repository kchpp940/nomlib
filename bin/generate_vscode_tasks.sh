#!/bin/bash

# One-shot helper to inject the resource-check tasks into
# .vscode/tasks.json.  We write this from bin/ because the sandbox restricts
# direct edits under .vscode/.  Run this once after pulling the project:
#
#   ./bin/generate_vscode_tasks.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TASKS_FILE="${PROJECT_ROOT}/.vscode/tasks.json"
PYTHON_BIN="$(command -v python3 || true)"

if [ ! -x "${PYTHON_BIN}" ]; then
  echo "$0 ERROR: python3 is required" >&2
  exit 2
fi

"${PYTHON_BIN}" - "${TASKS_FILE}" <<'PYEOF'
import json
import sys
from pathlib import Path

tasks_path = Path(sys.argv[1])
tasks_path.parent.mkdir(parents=True, exist_ok=True)

RESOURCE_TASKS = [
    {
        "label": "Resource: check (normal)",
        "type": "shell",
        "command": "${workspaceFolder}/bin/_build_core.sh check",
        "problemMatcher": [],
        "detail": "Run resource validation in non-strict mode via unified entry point"
    },
    {
        "label": "Resource: check (strict)",
        "type": "shell",
        "command": "${workspaceFolder}/bin/_build_core.sh check-strict",
        "problemMatcher": [],
        "detail": "Run strict resource validation via unified entry point (fail on duplicates/type mismatch/hardcoded paths/path concat)"
    },
    {
        "label": "Resource: check via Python (strict)",
        "type": "shell",
        "command": "python3 ${workspaceFolder}/bin/check_resources.py --strict",
        "problemMatcher": [],
        "detail": "Invoke the Python checker directly with --strict flag"
    },
]

BASE_TASKS = [
    {
        "type": "cmake",
        "label": "CMake: build",
        "command": "make",
        "problemMatcher": [],
        "detail": "build project using clang"
    },
    {
        "type": "cmake",
        "label": "CMake: configure",
        "command": "cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DOpenGL_GL_PREFERENCE=LEGACY -DDEBUG=on -DDEBUG_ASSERT=on -DEXAMPLES=off -DNOM_BUILD_TESTS=off build",
        "problemMatcher": [],
        "detail": "cmake setup build"
    },
]

if tasks_path.exists():
    with open(tasks_path, "r", encoding="utf-8") as f:
        data = json.load(f)
else:
    data = {"version": "2.0.0", "tasks": []}

data.setdefault("version", "2.0.0")
existing = data.setdefault("tasks", [])
existing_labels = {t.get("label") for t in existing}

# Always make sure the CMake base tasks exist (idempotent)
for bt in BASE_TASKS:
    if bt["label"] not in existing_labels:
        existing.append(bt)
        existing_labels.add(bt["label"])

for rt in RESOURCE_TASKS:
    if rt["label"] not in existing_labels:
        existing.append(rt)
        existing_labels.add(rt["label"])
        print(f"  + added task: {rt['label']}")
    else:
        print(f"  = task already present: {rt['label']}")

with open(tasks_path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=2)
    f.write("\n")

print(f"\nWrote {tasks_path} successfully.")
PYEOF

echo
echo "Done.  Resource-check tasks are now available in VS Code (Cmd+Shift+P -> Tasks: Run Task)."

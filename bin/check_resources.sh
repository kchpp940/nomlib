#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PYTHON_BIN=$(which python3)

if [ ! -x "${PYTHON_BIN}" ]; then
  echo "$0 ERROR: python3 command not found"
  exit 1
fi

exec "${PYTHON_BIN}" "${PROJECT_ROOT}/bin/check_resources.py" "$@"

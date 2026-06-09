#!/bin/bash

# NOTE: This script is a backwards-compatible shim. All logic now delegates to
# bin/_build_core.sh (the unified build entry point).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

exec "${PROJECT_ROOT}/bin/_build_core.sh" configure "$@"

#!/bin/bash

# Unified build entry point for nomlib. All configure/build scripts (both
# top-level bin/ and .vscode/bin/) eventually delegate here.
#
# Subcommands:
#   configure     Run CMake configure step (with resource pre-check in strict
#                 mode when NOM_STRICT_RESOURCE_CHECK=1).
#   build         Build the generated project (with resource pre-check).
#   check         Run the resource checker in non-strict (warning) mode.
#   check-strict  Run the resource checker in strict (fail-on-warning) mode.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PYTHON_BIN="$(command -v python3 || true)"
RESOURCE_CHECKER="${PROJECT_ROOT}/bin/check_resources.py"

run_resource_check() {
  local strict="$1"
  if [ ! -x "${PYTHON_BIN}" ]; then
    echo "[_build_core] WARNING: python3 not found - skipping resource checks"
    return 0
  fi
  if [ ! -f "${RESOURCE_CHECKER}" ]; then
    echo "[_build_core] WARNING: check_resources.py missing - skipping resource checks"
    return 0
  fi
  local extra_args=""
  if [ "${strict}" = "1" ]; then
    extra_args="--strict"
    echo "[_build_core] Running STRICT resource checks..."
  else
    echo "[_build_core] Running resource checks..."
  fi
  if ! "${PYTHON_BIN}" "${RESOURCE_CHECKER}" ${extra_args}; then
    echo "[_build_core] Resource checks FAILED - aborting."
    exit 1
  fi
  echo "[_build_core] Resource checks passed."
  echo
}

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [args...]

Commands:
  configure [cmake_args...]   Run CMake configure step.
  build [make_args...]        Build the project.
  check                       Run resource checker (non-strict).
  check-strict                Run resource checker (strict mode).
  -h, --help                  Show this help.

Environment:
  NOM_STRICT_RESOURCE_CHECK=1  Enable strict resource checks on configure/build.
  NOM_SKIP_RESOURCE_CHECK=1    Skip resource checks entirely.
EOF
}

cmd_configure() {
  if [ "${NOM_SKIP_RESOURCE_CHECK}" != "1" ]; then
    local strict="0"
    if [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ]; then
      strict="1"
    fi
    run_resource_check "${strict}"
  fi
  echo "[_build_core] Running cmake configure with args: $*"
  exec cmake "$@"
}

cmd_build() {
  if [ "${NOM_SKIP_RESOURCE_CHECK}" != "1" ]; then
    local strict="0"
    if [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ]; then
      strict="1"
    fi
    run_resource_check "${strict}"
  fi
  if command -v xcodebuild >/dev/null 2>&1 && ls -d *.xcodeproj >/dev/null 2>&1; then
    echo "[_build_core] Building with Xcode..."
    exec xcodebuild "$@"
  else
    echo "[_build_core] Building with make..."
    exec make "$@"
  fi
}

cmd_check() {
  run_resource_check 0
}

cmd_check_strict() {
  run_resource_check 1
}

if [ $# -eq 0 ]; then
  usage
  exit 2
fi

subcommand="$1"
shift

case "${subcommand}" in
  configure)
    cmd_configure "$@"
    ;;
  build)
    cmd_build "$@"
    ;;
  check)
    cmd_check
    ;;
  check-strict)
    cmd_check_strict
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "Unknown subcommand: ${subcommand}" >&2
    usage >&2
    exit 2
    ;;
esac

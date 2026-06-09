#!/bin/bash

# Unified build entry point for nomlib.
#
# DESIGN: This script is a thin dispatcher. It does NOT reimplement configure
# or build logic. For `configure` and `build` it simply runs the resource
# pre-check (respecting NOM_STRICT_RESOURCE_CHECK / NOM_SKIP_RESOURCE_CHECK)
# and then delegates to the original project scripts under bin/.
#
# New subcommands added incrementally:
#   check          Run resource checker in non-strict mode
#   check-strict   Run resource checker in strict (fail-on-warning) mode
#
# Delegated subcommands (pre-check + forward to original script):
#   configure      Forward to bin/configure.sh
#   build          Forward to bin/clang_build.sh (or bin/xcode_build.sh when applicable)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PYTHON_BIN="$(command -v python3 || true)"
RESOURCE_CHECKER="${PROJECT_ROOT}/bin/check_resources.py"

run_resource_check() {
  local strict="$1"
  if [ "${_NOM_BUILD_CORE_RUNNING}" = "1" ]; then
    return 0
  fi
  if [ "${NOM_SKIP_RESOURCE_CHECK}" = "1" ]; then
    return 0
  fi
  export _NOM_BUILD_CORE_RUNNING=1
  if [ ! -x "${PYTHON_BIN}" ]; then
    echo "[_build_core] WARNING: python3 not found - skipping resource checks"
    return 0
  fi
  if [ ! -f "${RESOURCE_CHECKER}" ]; then
    echo "[_build_core] WARNING: check_resources.py missing - skipping resource checks"
    return 0
  fi
  if [ "${strict}" = "1" ]; then
    echo "[_build_core] Running STRICT resource checks..."
    if ! "${PYTHON_BIN}" "${RESOURCE_CHECKER}" --strict; then
      echo "[_build_core] STRICT resource checks FAILED - aborting."
      exit 1
    fi
  else
    echo "[_build_core] Running resource checks..."
    if ! "${PYTHON_BIN}" "${RESOURCE_CHECKER}"; then
      echo "[_build_core] Resource checks FAILED - aborting."
      exit 1
    fi
  fi
  echo "[_build_core] Resource checks passed."
  echo
}

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [args...]

Incremental subcommands:
  check                       Run resource checker (non-strict).
  check-strict                Run resource checker (strict mode).

Delegated subcommands (pre-check + forward):
  configure [args...]         Run resource check, then delegate to bin/configure.sh.
  build [args...]             Run resource check, then delegate to bin/clang_build.sh.
  -h, --help                  Show this help.

Environment:
  NOM_STRICT_RESOURCE_CHECK=1  Enable strict resource checks on configure/build.
  NOM_SKIP_RESOURCE_CHECK=1    Skip resource checks entirely.
EOF
}

cmd_check() {
  local strict="0"
  [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ] && strict="1"
  run_resource_check "${strict}"
}

cmd_check_strict() {
  run_resource_check 1
}

cmd_configure() {
  local strict="0"
  [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ] && strict="1"
  run_resource_check "${strict}"
  echo "[_build_core] Delegating to bin/configure.sh"
  exec "${SCRIPT_DIR}/configure.sh" "$@"
}

cmd_build() {
  local strict="0"
  [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ] && strict="1"
  run_resource_check "${strict}"
  if command -v xcodebuild >/dev/null 2>&1 && ls -d *.xcodeproj >/dev/null 2>&1; then
    echo "[_build_core] Delegating to bin/xcode_build.sh"
    exec "${SCRIPT_DIR}/xcode_build.sh" "$@"
  else
    echo "[_build_core] Delegating to bin/clang_build.sh"
    exec "${SCRIPT_DIR}/clang_build.sh" "$@"
  fi
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

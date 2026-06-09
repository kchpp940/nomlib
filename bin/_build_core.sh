#!/bin/bash

# bin/_build_core.sh
#
# Unified build entry point for nomlib.
# All scripts under bin/ and .vscode/bin/ delegate to this file.
#
# Usage:
#   _build_core.sh <command> [options...]
#
# Commands:
#   configure   Run CMake configuration (preflight runs automatically before)
#   build       Build the project
#   install     Install built artifacts
#   clean       Remove build outputs
#   uninstall   Uninstall installed artifacts
#   distclean   Uninstall + clean + remove CMake cache
#   docs        Build Doxygen documentation
#   ctags       Generate ctags file
#
# To skip preflight during configure:
#   NOM_SKIP_PREFLIGHT=1 _build_core.sh configure ...

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PREFLIGHT_SCRIPT="${PROJECT_ROOT}/bin/preflight.sh"

VERBOSE=false
BUILD_TYPE="Debug"
GEN_PROJECT_TYPE=""
BUILD_INSTALL_DIR="${HOME}/Library/Frameworks"
BUILD_DIR="$(pwd)"
TESTS="on"
EXAMPLES="on"
DOCS="off"
NO_CACHE_CLEAR=false
NUM_THREADS="${NUM_THREADS:-}"
TARGET_ARCH=""

usage_core() {
  cat <<EOF
nomlib unified build core

Usage:
  $(basename "$0") <command> [options]

Commands:
  configure   Run CMake configuration (preflight runs automatically)
  build       Build the project
  install     Install built artifacts
  clean       Remove build outputs
  uninstall   Uninstall installed artifacts
  distclean   Uninstall + clean + clear CMake cache
  docs        Build Doxygen documentation
  ctags       Generate ctags tag file
  preflight   Run dependency preflight check only

Common Options:
  -d, --debug              Build type: Debug (default)
  -r, --release            Build type: Release
  -G <generator>           CMake generator (e.g. "Unix Makefiles", Xcode)
  --install-dir <path>     CMAKE_INSTALL_PREFIX
  --build-dir <path>       Build directory (default: current dir)
  --tests <on|off>         Build unit tests (default: on)
  --examples <on|off>      Build examples (default: on)
  --docs <on|off>          Build docs target (default: off)
  --arch <arch>            Target architecture (arm64 | x86_64 | auto)
  --no-cache-clear         Skip clearing CMake cache before configure
  -j <N>                   Parallel build jobs (or NUM_THREADS env)
  --skip-preflight         Skip the dependency preflight check
  -v, --verbose            Verbose output
  -h, --help               Show this help

Environment:
  NOM_SKIP_PREFLIGHT=1     Skip preflight (same as --skip-preflight)
  NUM_THREADS=N            Parallel build jobs (same as -j N)
EOF
}

# ---- Argument parsing ----

if [[ $# -lt 1 ]]; then
  usage_core
  exit 2
fi

COMMAND="$1"
shift

# Special case: 'preflight' command delegates all remaining args directly to
# bin/preflight.sh, so --help, --json, --fixes, etc. work as documented.
if [[ "$COMMAND" == "preflight" ]]; then
  exec "${PREFLIGHT_SCRIPT}" "$@"
fi

SKIP_PREFLIGHT="${NOM_SKIP_PREFLIGHT:-0}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -d|--debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    -r|--release)
      BUILD_TYPE="Release"
      shift
      ;;
    -G)
      if [[ -z "${2:-}" ]]; then
        echo "Error: -G requires a generator argument" >&2
        exit 2
      fi
      GEN_PROJECT_TYPE="$2"
      shift 2
      ;;
    --install-dir)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --install-dir requires a path argument" >&2
        exit 2
      fi
      BUILD_INSTALL_DIR="$2"
      shift 2
      ;;
    --build-dir)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --build-dir requires a path argument" >&2
        exit 2
      fi
      BUILD_DIR="$2"
      shift 2
      ;;
    --tests)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --tests requires on|off" >&2
        exit 2
      fi
      TESTS="$2"
      shift 2
      ;;
    --examples)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --examples requires on|off" >&2
        exit 2
      fi
      EXAMPLES="$2"
      shift 2
      ;;
    --docs)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --docs requires on|off" >&2
        exit 2
      fi
      DOCS="$2"
      shift 2
      ;;
    --arch)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --arch requires an argument" >&2
        exit 2
      fi
      TARGET_ARCH="$2"
      shift 2
      ;;
    --no-cache-clear)
      NO_CACHE_CLEAR=true
      shift
      ;;
    -j)
      if [[ -z "${2:-}" ]]; then
        echo "Error: -j requires a number argument" >&2
        exit 2
      fi
      NUM_THREADS="$2"
      shift 2
      ;;
    --skip-preflight)
      SKIP_PREFLIGHT=1
      shift
      ;;
    -v|--verbose)
      VERBOSE=true
      shift
      ;;
    -h|--help)
      usage_core
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      # Legacy positional argument support:
      #   configure.sh <Debug|Release> [generator] [install_dir]
      if [[ "$COMMAND" == "configure" ]]; then
        case "$1" in
          Debug|Release)
            BUILD_TYPE="$1"
            shift
            ;;
          *)
            if [[ -z "$GEN_PROJECT_TYPE" && ! -d "$1" ]]; then
              GEN_PROJECT_TYPE="$1"
              shift
            elif [[ -z "$GEN_PROJECT_TYPE" || -d "$1" ]]; then
              BUILD_INSTALL_DIR="$1"
              shift
            else
              echo "Unknown argument: $1" >&2
              usage_core
              exit 2
            fi
            ;;
        esac
      else
        echo "Unknown option: $1" >&2
        usage_core
        exit 2
      fi
      ;;
  esac
done

log_info()  { echo "$*"; }
log_verbose() {
  if [[ "$VERBOSE" == "true" ]]; then
    echo "  [VERBOSE] $*"
  fi
}

# ---- Preflight wrapper ----

cache_preflight_json() {
  local build_dir="$1"
  if [[ -z "$build_dir" || ! -d "$build_dir" ]]; then
    return 0
  fi
  local cache_file="${build_dir}/preflight_result.json"
  local pf_args=(--json)
  if [[ -n "$TARGET_ARCH" ]]; then
    pf_args+=(--arch "$TARGET_ARCH")
  fi
  "${PREFLIGHT_SCRIPT}" "${pf_args[@]}" > "${cache_file}" 2>/dev/null || true
  log_verbose "Preflight JSON cached at ${cache_file}"
}

run_preflight_check() {
  local build_dir="$1"
  if [[ "$SKIP_PREFLIGHT" == "1" ]]; then
    log_info "Skipping dependency preflight (NOM_SKIP_PREFLIGHT=1)."
    return 0
  fi
  if [[ ! -x "$PREFLIGHT_SCRIPT" ]]; then
    log_info "Preflight script not found or not executable; skipping."
    return 0
  fi

  # Always cache JSON so CMake, other bin scripts and README tooling can reuse
  cache_preflight_json "$build_dir"

  log_info ""
  log_info "=== Running dependency preflight ==="
  local preflight_args=()
  if [[ -n "$TARGET_ARCH" ]]; then
    preflight_args+=(--arch "$TARGET_ARCH")
  fi
  if [[ "$VERBOSE" == "true" ]]; then
    preflight_args+=(--verbose)
  fi

  if "${PREFLIGHT_SCRIPT}" "${preflight_args[@]}"; then
    log_info "Preflight: all checks passed."
    return 0
  fi

  local exit_code=$?
  log_info ""
  log_info "Preflight found issues (exit code ${exit_code})."
  if [[ -n "$build_dir" && -f "${build_dir}/preflight_result.json" ]]; then
    log_info "  Cached JSON:        ${build_dir}/preflight_result.json"
  fi
  log_info "  View detailed fixes:  ${PREFLIGHT_SCRIPT} --fixes"
  log_info "  Full report:          ${PREFLIGHT_SCRIPT}"
  log_info ""

  if [[ ! -t 0 ]]; then
    log_info "Non-interactive shell: aborting. Use NOM_SKIP_PREFLIGHT=1 to override."
    exit 1
  fi

  read -p "Continue with configure anyway? [y/N] " -n 1 -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    log_info "Aborted by user."
    exit 1
  fi
  log_info "Continuing (preflight overridden)."
  return 0
}

# ---- Command implementations ----

cmd_configure() {
  if [[ ! -d "$BUILD_DIR" ]]; then
    log_info "Creating build directory: ${BUILD_DIR}"
    mkdir -p "$BUILD_DIR"
  fi
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }

  if [[ "$NO_CACHE_CLEAR" != "true" ]]; then
    log_info "Clearing CMake cache..."
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
  fi

  # Run preflight BEFORE invoking CMake (pass BUILD_DIR for JSON caching)
  run_preflight_check "${BUILD_DIR}"

  local build_flags=""
  case "$BUILD_TYPE" in
    Debug)
      build_flags+=" -DDEBUG=on -DDEBUG_ASSERT=on"
      ;;
    Release)
      build_flags+=" -DDEBUG=off -DDEBUG_ASSERT=off"
      ;;
  esac

  build_flags+=" -DEXAMPLES=${EXAMPLES}"
  build_flags+=" -DNOM_BUILD_TESTS=${TESTS}"

  local gen_arg=()
  if [[ -n "$GEN_PROJECT_TYPE" ]]; then
    gen_arg=(-G"${GEN_PROJECT_TYPE}")
  fi

  local arch_args=()
  if [[ -n "$TARGET_ARCH" && "$TARGET_ARCH" != "auto" ]]; then
    arch_args+=(-DCMAKE_OSX_ARCHITECTURES="${TARGET_ARCH}")
  fi

  log_info ""
  log_info "=== Configuring ${BUILD_TYPE} build ==="
  log_info "  Install prefix: ${BUILD_INSTALL_DIR}"
  log_info "  Build dir:      ${BUILD_DIR}"
  log_info "  Tests:          ${TESTS}"
  log_info "  Examples:       ${EXAMPLES}"
  if [[ -n "$GEN_PROJECT_TYPE" ]]; then
    log_info "  Generator:      ${GEN_PROJECT_TYPE}"
  fi
  if [[ -n "$TARGET_ARCH" ]]; then
    log_info "  Target arch:    ${TARGET_ARCH}"
  fi
  log_info ""

  cmake ${gen_arg[@]+"${gen_arg[@]}"} \
    ${build_flags} \
    ${arch_args[@]+"${arch_args[@]}"} \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
    -DCMAKE_INSTALL_PREFIX="${BUILD_INSTALL_DIR}" \
    "${PROJECT_ROOT}"
}

cmd_build() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Building ${BUILD_TYPE} project..."
  local jobs_arg=()
  if [[ -n "$NUM_THREADS" ]]; then
    jobs_arg=(-j "$NUM_THREADS")
  fi
  make ${jobs_arg[@]+"${jobs_arg[@]}"}
}

cmd_install() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Installing ${BUILD_TYPE} project..."
  make install
}

cmd_clean() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Cleaning ${BUILD_TYPE} build..."
  make clean
}

cmd_uninstall() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Uninstalling..."
  make uninstall
}

cmd_distclean() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Running distclean (uninstall + clean + cache wipe)..."
  make uninstall 2>/dev/null || true
  make clean 2>/dev/null || true
  rm -f CMakeCache.txt
  rm -rf CMakeFiles
  log_info "distclean done."
}

cmd_docs() {
  cd "$BUILD_DIR" || {
    echo "Error: cannot cd to ${BUILD_DIR}" >&2
    exit 1
  }
  log_info "Building documentation..."
  make docs
}

cmd_ctags() {
  cd "$PROJECT_ROOT" || {
    echo "Error: cannot cd to ${PROJECT_ROOT}" >&2
    exit 1
  }
  log_info "Generating ctags..."
  local CTAGS_BIN
  CTAGS_BIN="$(command -v ctags || true)"
  if [[ -z "$CTAGS_BIN" ]]; then
    echo "Error: ctags not found in PATH" >&2
    exit 1
  fi
  "${CTAGS_BIN}" -R src include/nomlib/
}

# ---- Dispatch ----

case "$COMMAND" in
  configure) cmd_configure ;;
  build)     cmd_build ;;
  install)   cmd_install ;;
  clean)     cmd_clean ;;
  uninstall) cmd_uninstall ;;
  distclean) cmd_distclean ;;
  docs)      cmd_docs ;;
  ctags)     cmd_ctags ;;
  -h|--help) usage_core; exit 0 ;;
  *)
    echo "Unknown command: $COMMAND" >&2
    usage_core
    exit 2
    ;;
esac

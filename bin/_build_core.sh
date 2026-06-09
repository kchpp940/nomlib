#!/bin/bash

shopt -s nocasematch

NOMLIB_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NOMLIB_ROOT_DIR="$(cd "${NOMLIB_SCRIPT_DIR}/.." && pwd)"

nomlib_detect_project_root() {
  local dir
  dir="$(pwd)"
  while [[ "$dir" != "/" ]]; do
    if [[ -f "$dir/CMakeLists.txt" && -d "$dir/src" && -d "$dir/include" ]]; then
      echo "$dir"
      return 0
    fi
    dir="$(dirname "$dir")"
  done
  echo "${NOMLIB_ROOT_DIR}"
}

NOMLIB_PROJECT_ROOT="$(nomlib_detect_project_root)"

nomlib_usage() {
  local cmd="$1"
  echo "Usage: ${cmd} [options]"
  echo
  echo "Options:"
  echo "  --debug, -d             Build type: Debug (default)"
  echo "  --release, -r           Build type: Release"
  echo "  -G <generator>          CMake generator (e.g. Xcode, \"Unix Makefiles\")"
  echo "  --install-dir <path>    CMAKE_INSTALL_PREFIX"
  echo "  --build-dir <path>      Build directory (default: current dir)"
  echo "  --tests <on|off>        Build tests (default: on)"
  echo "  --examples <on|off>     Build examples (default: on)"
  echo "  --docs <on|off>         Build docs (default: off)"
  echo "  --no-cache-clear        Skip clearing CMake cache before configure"
  echo "  -j <N>                  Parallel build jobs (or set NUM_THREADS env)"
  echo "  -h, --help              Show this help"
  echo
  echo "Positional (legacy):"
  echo "  ${cmd} <Debug|Release> [generator] [install_dir]"
  exit 0
}

nomlib_parse_args() {
  NOM_BUILD_TYPE="Debug"
  NOM_GENERATOR=""
  NOM_INSTALL_DIR=""
  NOM_BUILD_DIR="$(pwd)"
  NOM_TESTS="on"
  NOM_EXAMPLES="on"
  NOM_DOCS="off"
  NOM_CLEAR_CACHE=1
  NOM_NUM_THREADS="${NUM_THREADS:-}"
  NOM_CHECK_JSON=1

  while [[ $# -gt 0 ]]; do
    case "$1" in
      -h|--help)
        nomlib_usage "$(basename "$0")"
        ;;
      --debug|-d)
        NOM_BUILD_TYPE="Debug"
        shift
        ;;
      --release|-r)
        NOM_BUILD_TYPE="Release"
        shift
        ;;
      -G)
        NOM_GENERATOR="$2"
        shift 2
        ;;
      --install-dir)
        NOM_INSTALL_DIR="$2"
        shift 2
        ;;
      --build-dir)
        NOM_BUILD_DIR="$2"
        shift 2
        ;;
      --tests)
        NOM_TESTS="$2"
        shift 2
        ;;
      --examples)
        NOM_EXAMPLES="$2"
        shift 2
        ;;
      --docs)
        NOM_DOCS="$2"
        shift 2
        ;;
      --no-cache-clear)
        NOM_CLEAR_CACHE=0
        shift
        ;;
      -j)
        NOM_NUM_THREADS="$2"
        shift 2
        ;;
      Debug|Release|debug|release)
        NOM_BUILD_TYPE="$1"
        shift
        ;;
      *)
        if [[ -z "$NOM_GENERATOR" && "$1" == -* ]]; then
          echo "Unknown option: $1" >&2
          exit 1
        fi
        if [[ -z "$NOM_GENERATOR" ]]; then
          NOM_GENERATOR="$1"
        elif [[ -z "$NOM_INSTALL_DIR" ]]; then
          NOM_INSTALL_DIR="$1"
        else
          echo "Unexpected argument: $1" >&2
          exit 1
        fi
        shift
        ;;
    esac
  done

  case "$NOM_BUILD_TYPE" in
    debug) NOM_BUILD_TYPE="Debug" ;;
    release) NOM_BUILD_TYPE="Release" ;;
  esac

  if [[ -z "$NOM_INSTALL_DIR" ]]; then
    case "$(uname -s)" in
      Darwin)
        NOM_INSTALL_DIR="${HOME}/Library/Frameworks"
        ;;
      Linux)
        NOM_INSTALL_DIR="/usr/local"
        ;;
      *)
        NOM_INSTALL_DIR="$(pwd)"
        ;;
    esac
  fi
}

nomlib_clear_cmake_cache() {
  local build_dir="$1"
  echo "Clearing CMake cache..."
  [[ -f "${build_dir}/CMakeCache.txt" ]] && rm -f "${build_dir}/CMakeCache.txt"
  [[ -d "${build_dir}/CMakeFiles" ]] && rm -rf "${build_dir}/CMakeFiles"
}

nomlib_build_flags() {
  local flags=""

  case "$NOM_BUILD_TYPE" in
    Debug)
      flags+=" -DDEBUG=on -DDEBUG_ASSERT=on"
      ;;
    Release)
      flags+=" -DDEBUG=off -DDEBUG_ASSERT=off"
      ;;
  esac

  flags+=" -DEXAMPLES=${NOM_EXAMPLES}"
  flags+=" -DNOM_BUILD_TESTS=${NOM_TESTS}"
  flags+=" -DDOCS=${NOM_DOCS}"

  case "$(uname -s)" in
    Darwin)
      flags+=" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7"
      ;;
    Linux)
      flags+=" -DOpenGL_GL_PREFERENCE=LEGACY"
      if [[ -z "$NOMLIB_DEPS_ROOT" ]]; then
        flags+=" -DNOMLIB_DEPS_PREFIX=${NOMLIB_PROJECT_ROOT}/third-party/osx"
      else
        flags+=" -DNOMLIB_DEPS_PREFIX=${NOMLIB_DEPS_ROOT}"
      fi
      ;;
  esac

  if [[ -n "$NOMLIB_DEPS_ROOT" ]]; then
    flags+=" -DNOMLIB_DEPS_PREFIX=${NOMLIB_DEPS_ROOT}"
  fi

  echo "$flags"
}

nomlib_cmd_configure() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  mkdir -p "$build_dir"
  cd "$build_dir" || exit 1

  if [[ "$NOM_CLEAR_CACHE" -eq 1 ]]; then
    nomlib_clear_cmake_cache "$build_dir"
  fi

  local flags
  flags="$(nomlib_build_flags)"

  local generator_arg=""
  if [[ -n "$NOM_GENERATOR" ]]; then
    generator_arg="-G${NOM_GENERATOR}"
  fi

  echo "Generating ${NOM_BUILD_TYPE} project files..."
  echo "BUILD_INSTALL_DIR: ${NOM_INSTALL_DIR}"
  echo "BUILD_FLAGS=${flags}"

  cmake $generator_arg $flags \
    -DCMAKE_INSTALL_PREFIX="${NOM_INSTALL_DIR}" \
    "${NOMLIB_PROJECT_ROOT}"
}

nomlib_cmd_build() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  local jobs_arg=""
  if [[ -n "$NOM_NUM_THREADS" ]]; then
    jobs_arg="-j ${NOM_NUM_THREADS}"
  fi

  if [[ "$NOM_CHECK_JSON" -eq 1 && -x "${NOMLIB_SCRIPT_DIR}/../.vscode/bin/check_json.sh" ]]; then
    "${NOMLIB_SCRIPT_DIR}/../.vscode/bin/check_json.sh"
    if [[ $? -ne 0 ]]; then
      echo "ERROR: JSON validation failure has prevented building the project." >&2
      exit 1
    fi
  fi

  echo "Building ${NOM_BUILD_TYPE} project... [target: build]"
  make $jobs_arg
}

nomlib_cmd_clean() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  echo "Cleaning ${NOM_BUILD_TYPE} project... [target: clean]"
  make clean
}

nomlib_cmd_install() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  echo "Installing ${NOM_BUILD_TYPE} project... [target: install]"
  make install
}

nomlib_cmd_uninstall() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  echo "Uninstalling ${NOM_BUILD_TYPE} project... [target: uninstall]"
  make uninstall
}

nomlib_cmd_distclean() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  echo "...Cleaning up development build environment..."
  echo "  BUILD_INSTALL_DIR: ${NOM_INSTALL_DIR}"

  make uninstall 2>/dev/null || true
  make clean 2>/dev/null || true

  nomlib_clear_cmake_cache "$build_dir"

  [[ -f "${build_dir}/install_manifest.txt" ]] && rm -f "${build_dir}/install_manifest.txt"
}

nomlib_cmd_docs() {
  nomlib_parse_args "$@"

  local build_dir="$NOM_BUILD_DIR"
  cd "$build_dir" || exit 1

  echo "Building ${NOM_BUILD_TYPE} project... [target: docs]"
  make docs

  if command -v pow >/dev/null 2>&1; then
    "${NOMLIB_SCRIPT_DIR}/gen_docs.sh" nomlib-docs
  fi
}

nomlib_cmd_ctags() {
  local root="$NOMLIB_PROJECT_ROOT"
  local source_dir="${root}/src"
  local header_dir="${root}/include/nomlib/"
  local tags_file="${root}/.tags"

  echo "Generating tag file: ${tags_file}"
  ctags -f "${tags_file}" -R "${source_dir}" "${header_dir}"
}

nomlib_dispatch() {
  local cmd="$1"
  shift
  case "$cmd" in
    configure) nomlib_cmd_configure "$@" ;;
    build)     nomlib_cmd_build "$@" ;;
    clean)     nomlib_cmd_clean "$@" ;;
    install)   nomlib_cmd_install "$@" ;;
    uninstall) nomlib_cmd_uninstall "$@" ;;
    distclean) nomlib_cmd_distclean "$@" ;;
    docs)      nomlib_cmd_docs "$@" ;;
    ctags)     nomlib_cmd_ctags "$@" ;;
    *)
      echo "Unknown command: $cmd" >&2
      exit 1
      ;;
  esac
}

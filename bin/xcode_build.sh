#!/bin/sh

# NOTE: This script is intended to be ran from the project's current build
# directory.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PYTHON_BIN=$(which python3)

if [ -x "${PYTHON_BIN}" ] && [ -f "${PROJECT_ROOT}/bin/check_resources.py" ]; then
  echo "Running pre-build resource checks..."
  "${PYTHON_BIN}" "${PROJECT_ROOT}/bin/check_resources.py"
  if [ $? -ne 0 ]; then
    echo "Resource checks FAILED - aborting build."
    exit 1
  fi
  echo "Resource checks passed."
  echo
fi

XCODEBUILD_BIN=$(which xcodebuild)
BUILD_TYPE_ARG=$1

# Default
BUILD_TYPE="Debug"

if [[ !( -z "${BUILD_TYPE_ARG}") ]]; then
  BUILD_TYPE=$1
fi

if [[ !( -z "${NUM_THREADS}") ]]; then
  NUM_THREADS_ARG="-jobs ${NUM_THREADS}"
fi
# echo "NUM_THREADS_ARG: ${NUM_THREADS_ARG}"

echo "Building ${BUILD_TYPE} project... [target: build]"
${XCODEBUILD_BIN} ${NUM_THREADS_ARG} -configuration ${BUILD_TYPE} \
-target ALL_BUILD

#!/bin/sh

# NOTE: This script is intended to be ran from the project's current build
# directory.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ "${_NOM_BUILD_CORE_RUNNING}" != "1" ] && [ "${NOM_SKIP_RESOURCE_CHECK}" != "1" ] && [ -x "${SCRIPT_DIR}/_build_core.sh" ]; then
  if [ "${NOM_STRICT_RESOURCE_CHECK}" = "1" ]; then
    "${SCRIPT_DIR}/_build_core.sh" check-strict || exit 1
  else
    "${SCRIPT_DIR}/_build_core.sh" check || exit 1
  fi
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

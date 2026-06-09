#!/usr/bin/env bash
# =============================================================================
# nomlib install/package verification script
#
# Usage:
#   ./cmake/scripts/verify_install.sh [--framework] [--prefix TMPDIR] [--extended]
#
# Runs end-to-end verification of nomlib's CMake install/package flow:
#   1. Configure + build + install nomlib into a temporary prefix
#   2. Check the install tree for completeness (libs, headers, Resources,
#      CMake package files)
#   3. Verify nomlib-targets.cmake only contains ENABLED modules (disabled
#      modules must NOT appear)
#   4. Consume the installed package from a minimal external CMake project
#      using find_package(nomlib CONFIG COMPONENTS ...) and assert that
#      ENABLED components are found and DISABLED components are NOT found
#   5. Verify the legacy find-module (Resources/CMake/nomlib-config.cmake)
#      correctly delegates to the modern config
#   6. Build + run the consumer executable and confirm RPATH lets it locate
#      nomlib dylibs at runtime
#   7. Exercise the 'uninstall' target
#   8. Stage a CPack run (if --cpack) and ensure exit 0
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

FRAMEWORK=OFF
PREFIX=""
BUILD_AUDIO=OFF
BUILD_GUI=OFF
BUILD_SERIALIZERS=OFF
BUILD_PTREE=OFF
BUILD_SYSTEM=ON
BUILD_GRAPHICS=OFF
BUILD_ACTIONS=OFF
RUN_CPACK=NO

while [[ $# -gt 0 ]]; do
  case "$1" in
    --framework)
      FRAMEWORK=ON
      shift
      ;;
    --prefix)
      PREFIX="$2"
      shift 2
      ;;
    --extended)
      BUILD_SYSTEM=ON
      BUILD_GRAPHICS=ON
      shift
      ;;
    --all-modules)
      BUILD_AUDIO=ON
      BUILD_GUI=ON
      BUILD_SERIALIZERS=ON
      BUILD_PTREE=ON
      BUILD_SYSTEM=ON
      BUILD_GRAPHICS=ON
      BUILD_ACTIONS=ON
      shift
      ;;
    --cpack)
      RUN_CPACK=YES
      shift
      ;;
    -h|--help)
      echo "Usage: $0 [--framework] [--prefix DIR] [--extended] [--all-modules] [--cpack]"
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

if [[ -z "${PREFIX}" ]]; then
  PREFIX="$(mktemp -d /tmp/nomlib-verify-XXXXXX)"
  echo "[verify] Using auto-generated install prefix: ${PREFIX}"
fi

BUILD_DIR="${PREFIX}/build"
CONSUMER_DIR="${REPO_ROOT}/cmake/test_consumer"
CONSUMER_BUILD_DIR="${PREFIX}/consumer-build"

# --- Build the enabled-components list (pass to consumer test) ---
ENABLED_COMPS="core;math;file"
DISABLED_COMPS="audio;gui;serializers;ptree;actions;extra_rescale_algo"

[[ "${BUILD_SYSTEM}"      == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};system"      || DISABLED_COMPS="${DISABLED_COMPS};system"
[[ "${BUILD_GRAPHICS}"    == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};graphics"    || DISABLED_COMPS="${DISABLED_COMPS};graphics"
[[ "${BUILD_AUDIO}"       == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};audio"       || :
[[ "${BUILD_GUI}"         == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};gui"         || :
[[ "${BUILD_SERIALIZERS}" == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};serializers" || :
[[ "${BUILD_PTREE}"       == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};ptree"       || :
[[ "${BUILD_ACTIONS}"     == "ON" ]] && ENABLED_COMPS="${ENABLED_COMPS};actions"     || :

echo ""
echo "============================================================"
echo " nomlib install verification"
echo "   FRAMEWORK     = ${FRAMEWORK}"
echo "   PREFIX        = ${PREFIX}"
echo "   SOURCE        = ${REPO_ROOT}"
echo "   ENABLED_COMPS = ${ENABLED_COMPS}"
echo "   DISABLED_COMPS= ${DISABLED_COMPS}"
echo "============================================================"
echo ""

pass()  { printf "  \033[32m[PASS]\033[0m %s\n" "$*"; }
fail()  { printf "  \033[31m[FAIL]\033[0m %s\n" "$*"; exit 1; }
info()  { printf "  \033[34m[INFO]\033[0m %s\n" "$*"; }
step()  { printf "\n\033[1m-- Step %d: %s\033[0m\n" "$1" "$2"; }

NATIVE_ARCH="$(uname -m)"

step 1 "Configure nomlib"
mkdir -p "${BUILD_DIR}"
cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DCMAKE_OSX_ARCHITECTURES="${NATIVE_ARCH}" \
  -DFRAMEWORK="${FRAMEWORK}" \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF \
  -DNOM_BUILD_AUDIO_UNIT="${BUILD_AUDIO}" \
  -DNOM_BUILD_GUI_UNIT="${BUILD_GUI}" \
  -DNOM_BUILD_SERIALIZERS_UNIT="${BUILD_SERIALIZERS}" \
  -DNOM_BUILD_PTREE_UNIT="${BUILD_PTREE}" \
  -DNOM_BUILD_SYSTEM_UNIT="${BUILD_SYSTEM}" \
  -DNOM_BUILD_GRAPHICS_UNIT="${BUILD_GRAPHICS}" \
  -DNOM_BUILD_ACTIONS_UNIT="${BUILD_ACTIONS}" \
  -DNOM_BUILD_EXTRA_RESCALE_ALGO_UNIT=OFF \
  >"${PREFIX}/cmake-configure.log" 2>&1 \
  || fail "cmake configure failed (see ${PREFIX}/cmake-configure.log)"
pass "Configure successful"

step 2 "Build nomlib"
cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 2)" \
  >"${PREFIX}/cmake-build.log" 2>&1 \
  || fail "cmake build failed (see ${PREFIX}/cmake-build.log)"
pass "Build successful"

step 3 "Install to prefix"
cmake --install "${BUILD_DIR}" >"${PREFIX}/cmake-install.log" 2>&1 \
  || fail "cmake install failed (see ${PREFIX}/cmake-install.log)"
pass "Install successful"

step 4 "Inspect install tree"

if [[ "${FRAMEWORK}" == "ON" ]]; then
  CONFIG_DIR="${PREFIX}/nomlib.framework/Resources/CMake"
  INCLUDE_DIR="${PREFIX}/nomlib.framework/Headers"
  RES_DIR="${PREFIX}/nomlib.framework/Resources"
  LEGACY_DIR="${PREFIX}/share/nomlib/CMake"
  LIB_CHECK=("${PREFIX}/nomlib-core.framework/nomlib-core")
else
  CONFIG_DIR="${PREFIX}/lib/cmake/nomlib"
  INCLUDE_DIR="${PREFIX}/include"
  RES_DIR="${PREFIX}/share/nomlib"
  LEGACY_DIR="${RES_DIR}/CMake"
  LIB_CHECK=( "${PREFIX}/lib/libnomlib-core.dylib"
              "${PREFIX}/lib/libnomlib-math.dylib"
              "${PREFIX}/lib/libnomlib-file.dylib" )
  [[ "${BUILD_SYSTEM}"   == "ON" ]] && LIB_CHECK+=( "${PREFIX}/lib/libnomlib-system.dylib" )
  [[ "${BUILD_GRAPHICS}" == "ON" ]] && LIB_CHECK+=( "${PREFIX}/lib/libnomlib-graphics.dylib" )
fi

# --- Required package files ---
for f in \
  "${CONFIG_DIR}/nomlib-config.cmake" \
  "${CONFIG_DIR}/nomlib-config-version.cmake" \
  "${CONFIG_DIR}/nomlib-targets.cmake"
do
  [[ -f "$f" ]] || fail "missing file: $f"
done
pass "CMake package config files present in ${CONFIG_DIR}"

# --- Libraries ---
for f in "${LIB_CHECK[@]}"; do
  if [[ "${FRAMEWORK}" == "OFF" ]]; then
    [[ -f "$f" ]] || fail "missing library: $f"
  fi
done
if [[ "${FRAMEWORK}" == "OFF" ]]; then
  pass "Libraries present: ${LIB_CHECK[*]}"
fi

# --- Headers ---
[[ -f "${INCLUDE_DIR}/nomlib/core.hpp" ]] || fail "missing header: ${INCLUDE_DIR}/nomlib/core.hpp"
[[ -f "${INCLUDE_DIR}/nomlib/math.hpp" ]] || fail "missing header: ${INCLUDE_DIR}/nomlib/math.hpp"
[[ -f "${INCLUDE_DIR}/nomlib/platforms.hpp" ]] || fail "missing header: ${INCLUDE_DIR}/nomlib/platforms.hpp"
if [[ "${BUILD_SYSTEM}" == "ON" ]]; then
  [[ -f "${INCLUDE_DIR}/nomlib/system.hpp" ]] || fail "missing header: ${INCLUDE_DIR}/nomlib/system.hpp"
fi
if [[ "${BUILD_GRAPHICS}" == "ON" ]]; then
  [[ -f "${INCLUDE_DIR}/nomlib/graphics.hpp" ]] || fail "missing header: ${INCLUDE_DIR}/nomlib/graphics.hpp"
fi
pass "Headers present in ${INCLUDE_DIR}/nomlib/"

# --- Resources ---
[[ -d "${RES_DIR}/SharedSupport/InputDevices" ]] \
  || fail "missing resources: ${RES_DIR}/SharedSupport/InputDevices"
[[ -f "${RES_DIR}/SharedSupport/InputDevices/gamecontrollerdb.txt" ]] \
  || fail "missing resource file: gamecontrollerdb.txt"
pass "Resources present in ${RES_DIR}"

# --- Legacy find-module (installed at share/nomlib/CMake) ---
LEGACY_CONFIG="${LEGACY_DIR}/nomlib-config.cmake"
[[ -f "${LEGACY_CONFIG}" ]] || fail "missing legacy wrapper: ${LEGACY_CONFIG}"
pass "Legacy wrapper present at ${LEGACY_CONFIG}"

step 5 "Verify nomlib-targets.cmake module export correctness"
# The export set should ONLY contain modules that were enabled at build time.
# Disabled modules must not appear.
TARGETS_FILE="${CONFIG_DIR}/nomlib-targets.cmake"

for _c in ${ENABLED_COMPS//;/ }; do
  # The targets file references the release build with filename:
  #   IMPORTED_LOCATION_RELEASE ".../libnomlib-<comp>..."
  #   or framework: ".../nomlib-<comp>.framework/..."
  if grep -q "nomlib-${_c}" "${TARGETS_FILE}"; then
    pass "  nomlib-${_c}: exported in nomlib-targets.cmake (as expected)"
  else
    fail "nomlib-${_c} NOT found in nomlib-targets.cmake but it was ENABLED!"
  fi
done

for _c in ${DISABLED_COMPS//;/ }; do
  if grep -q "nomlib-${_c}" "${TARGETS_FILE}"; then
    fail "nomlib-${_c} found in nomlib-targets.cmake but it was DISABLED!"
  else
    pass "  nomlib-${_c}: correctly absent from nomlib-targets.cmake"
  fi
done

step 6 "Consume via find_package(nomlib CONFIG COMPONENTS ...) from external project"
rm -rf "${CONSUMER_BUILD_DIR}"
mkdir -p "${CONSUMER_BUILD_DIR}"

cmake -S "${CONSUMER_DIR}" -B "${CONSUMER_BUILD_DIR}" \
  -DCMAKE_OSX_ARCHITECTURES="${NATIVE_ARCH}" \
  -Dnomlib_DIR="${CONFIG_DIR}" \
  -DTEST_COMPONENTS="${ENABLED_COMPS}" \
  -DTEST_MISSING_COMPONENTS="${DISABLED_COMPS}" \
  >"${PREFIX}/consumer-configure.log" 2>&1 \
  || fail "consumer configure failed (see ${PREFIX}/consumer-configure.log)"
pass "Consumer project configure OK (COMPONENTS=${ENABLED_COMPS})"

cmake --build "${CONSUMER_BUILD_DIR}" \
  >"${PREFIX}/consumer-build.log" 2>&1 \
  || fail "consumer build failed (see ${PREFIX}/consumer-build.log)"
pass "Consumer project build OK"

step 7 "Run consumer executable (validates RPATH + runtime linkage)"
CONSUMER_BIN="${CONSUMER_BUILD_DIR}/test_consumer"
if [[ ! -x "${CONSUMER_BIN}" ]]; then
  fail "consumer binary not found at ${CONSUMER_BIN}"
fi
"${CONSUMER_BIN}" >"${PREFIX}/consumer-run.log" 2>&1 \
  || fail "consumer run failed (see ${PREFIX}/consumer-run.log)"
grep -q "nomlib consumer test: OK" "${PREFIX}/consumer-run.log" \
  || fail "consumer did not output expected marker"
pass "Consumer runs correctly; RPATH resolves nomlib at runtime"

step 8 "Legacy wrapper delegation test"
LEGACY_CONSUMER_BUILD="${PREFIX}/legacy-consumer-build"
rm -rf "${LEGACY_CONSUMER_BUILD}"
mkdir -p "${LEGACY_CONSUMER_BUILD}"
cmake -S "${CONSUMER_DIR}" -B "${LEGACY_CONSUMER_BUILD}" \
  -DCMAKE_OSX_ARCHITECTURES="${NATIVE_ARCH}" \
  -DCMAKE_MODULE_PATH="${LEGACY_DIR}" \
  -Dnomlib_LEGACY_MODE=ON \
  -DTEST_COMPONENTS="core;math" \
  -DTEST_MISSING_COMPONENTS="" \
  >"${PREFIX}/legacy-configure.log" 2>&1 \
  || fail "legacy consumer configure failed (see ${PREFIX}/legacy-configure.log)"
grep -q "Delegating to modern package config" "${PREFIX}/legacy-configure.log" \
  || fail "legacy wrapper did not print delegation message"
cmake --build "${LEGACY_CONSUMER_BUILD}" \
  >"${PREFIX}/legacy-build.log" 2>&1 \
  || fail "legacy consumer build failed (see ${PREFIX}/legacy-build.log)"
"${LEGACY_CONSUMER_BUILD}/test_consumer" >/dev/null \
  || fail "legacy consumer binary failed to run"
pass "Legacy find-module correctly delegates to modern config"

step 9 "Run uninstall target"
cmake --build "${BUILD_DIR}" --target uninstall \
  >"${PREFIX}/cmake-uninstall.log" 2>&1 \
  || fail "uninstall target failed (see ${PREFIX}/cmake-uninstall.log)"

if [[ "${FRAMEWORK}" == "OFF" ]]; then
  [[ -f "${CONFIG_DIR}/nomlib-targets.cmake" ]] \
    && fail "uninstall did not remove nomlib-targets.cmake"
fi
pass "Uninstall target runs successfully"

step 10 "CPack run (if requested) - MUST exit 0"
if [[ "${RUN_CPACK}" == "YES" ]]; then
  # Re-install because step 9 uninstalled everything (cpack needs installed tree)
  cmake --install "${BUILD_DIR}" >"${PREFIX}/cmake-install2.log" 2>&1 \
    || fail "cmake re-install before cpack failed (see ${PREFIX}/cmake-install2.log)"
  cd "${BUILD_DIR}"
  cpack -C Release --config CPackConfig.cmake \
    >"${PREFIX}/cpack.log" 2>&1 \
    || fail "cpack failed (see ${PREFIX}/cpack.log) - MUST exit 0"
  # Verify that at least one package artifact was produced
  PKG_COUNT="$(find "${BUILD_DIR}" -maxdepth 1 \( -name '*.dmg' -o -name '*.tar.gz' -o -name '*.zip' \) 2>/dev/null | wc -l | tr -d ' ')"
  if [[ "${PKG_COUNT}" -ge 1 ]]; then
    pass "CPack produced ${PKG_COUNT} package artifact(s) in ${BUILD_DIR}"
  else
    info "cpack exit 0 but no .dmg/.tar.gz/.zip found in ${BUILD_DIR} (check generator)"
  fi
  pass "CPack run succeeded (exit 0)"
else
  info "Skipping CPack (pass --cpack to enable; it must exit 0)"
fi

echo ""
echo "============================================================"
printf "\033[32m  All verification steps passed!\033[0m\n"
echo "  Install prefix: ${PREFIX}"
echo "============================================================"

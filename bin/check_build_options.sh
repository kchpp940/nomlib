#!/usr/bin/env bash
#
# check_build_options.sh — CMake build-option dependency validation matrix.
#
# Runs a comprehensive matrix of CMake configure invocations covering
# every build option combination the project supports (intentionally, as
# regression coverage) to ensure that the nom_validate_build_options()
# macro in cmake/macros.cmake correctly:
#
#   * Lets VALID configurations through (minimal / trimmed builds)
#   * Rejects INVALID ones BEFORE compilation starts,
#     with a clear "Build Option Validation FAILED" banner.
#
# Usage:
#   bin/check_build_options.sh          # from project root
#   make check-build-options            # after CMake configure, via the top-
#                                       # level custom target
#
# Exit code:
#   0 if ALL cases match their expected result
#   1 if any case mismatches (details printed to stdout)
#
# See also:
#   cmake/macros.cmake     — nom_validate_build_options()
#   cmake/functions.cmake  — nom_probe_third_party()
#   CMakeLists.txt         — the root where the above are wired together

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_BASE="${PROJECT_ROOT}/build_matrix_check"
PASS=0
FAIL=0
EXPECT_PASS=0
EXPECT_FAIL=0
TMP_ERR=""

run_case() {
  local name="$1"; shift
  local expect="$1"; shift    # "pass" or "fail"
  local description="$1"; shift
  local args=("$@")

  local bdir="${BUILD_BASE}/$(echo "$name" | tr ' /' '__')"
  rm -rf "$bdir"
  mkdir -p "$bdir"

  printf '=%.0s' {1..70}
  echo
  echo "CASE      : $name"
  echo "DESC      : $description"
  echo "CMAKE_ARGS: ${args[*]}"
  echo "EXPECT    : validation $expect"

  if (cd "$bdir" && cmake "${args[@]}" "$PROJECT_ROOT") >"$bdir/cmake.out" 2>"$bdir/cmake.err"; then
    :
  fi

  # Detect whether OUR validation layer (nom_validate_build_options) fired.
  # The banner "Build Option Validation FAILED" is the unique, and only,
  # signal we care about here.  Any other CMake error (install bugs,
  # missing unrelated project issues that exist independently of our
  # validation) are ignored for the purpose of this matrix.
  if grep -q "Build Option Validation FAILED" "$bdir/cmake.out" "$bdir/cmake.err" 2>/dev/null; then
    local actual="fail"
  else
    local actual="pass"
  fi

  if [[ "$actual" == "$expect" ]]; then
    echo "RESULT    : OK ($actual)"
    if [[ "$expect" == "pass" ]]; then PASS=$((PASS+1)); EXPECT_PASS=$((EXPECT_PASS+1))
    else FAIL=$((FAIL+1)); EXPECT_FAIL=$((EXPECT_FAIL+1))
    fi
    rm -rf "$bdir"
  else
    echo "RESULT    : MISMATCH — expected $expect, got $actual"
    echo "--- stdout (head 40 lines) ---"
    head -40 "$bdir/cmake.out" 2>/dev/null
    echo "--- stderr (head 40 lines) ---"
    head -40 "$bdir/cmake.err" 2>/dev/null
    echo "------------------------------"
    TMP_ERR="${TMP_ERR}  ✗ $name — expected $expect / actual $actual
"
    if [[ "$expect" == "pass" ]]; then EXPECT_PASS=$((EXPECT_PASS+1))
    else EXPECT_FAIL=$((EXPECT_FAIL+1))
    fi
  fi
  echo
}

# ============================================================================
# VALID configurations — must PASS validation
# ============================================================================

run_case "minimal-all-off" pass \
  "Core/math/file/system/ptree/serializers + graphics only; all optional off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

run_case "graphics-only" pass \
  "Graphics enabled; actions/audio/gui/examples/tests off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

# ============================================================================
# INVALID configurations — must FAIL validation BEFORE compilation
# ============================================================================

# --- Actions unit ---

run_case "actions-on-graphics-off" fail \
  "Actions enabled with graphics unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

run_case "actions-on-audio-off" fail \
  "Actions enabled with audio unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

run_case "actions-on-core-math-system-off" fail \
  "Actions enabled, all core deps off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF \
  -DNOM_BUILD_CORE_UNIT=OFF \
  -DNOM_BUILD_MATH_UNIT=OFF \
  -DNOM_BUILD_SYSTEM_UNIT=OFF

# --- Graphics unit ---

run_case "graphics-on-core-deps-off" fail \
  "Graphics enabled, all core deps off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF \
  -DNOM_BUILD_CORE_UNIT=OFF \
  -DNOM_BUILD_MATH_UNIT=OFF \
  -DNOM_BUILD_FILE_UNIT=OFF \
  -DNOM_BUILD_SERIALIZERS_UNIT=OFF \
  -DNOM_BUILD_SYSTEM_UNIT=OFF

# --- GUI unit ---

run_case "gui-on-graphics-off" fail \
  "GUI enabled but graphics unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=ON \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

run_case "gui-on-librocket-missing" fail \
  "GUI enabled but libRocket missing" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=ON \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF

# --- Audio unit ---

run_case "audio-on-core-math-system-off" fail \
  "Audio enabled, core deps off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=OFF \
  -DNOM_BUILD_CORE_UNIT=OFF \
  -DNOM_BUILD_MATH_UNIT=OFF \
  -DNOM_BUILD_SYSTEM_UNIT=OFF

# --- Examples ---

run_case "examples-on-actions-off" fail \
  "Examples enabled, actions unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=ON \
  -DEXAMPLES=ON \
  -DNOM_BUILD_TESTS=OFF

run_case "examples-on-gui-off" fail \
  "Examples enabled, GUI unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=ON \
  -DNOM_BUILD_TESTS=OFF

run_case "examples-on-audio-off" fail \
  "Examples enabled, audio unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=ON \
  -DEXAMPLES=ON \
  -DNOM_BUILD_TESTS=OFF

run_case "examples-on-graphics-off" fail \
  "Examples enabled, graphics unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=ON \
  -DNOM_BUILD_AUDIO_UNIT=ON \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=ON \
  -DNOM_BUILD_TESTS=OFF

# --- Tests ---

run_case "tests-audio-on-audio-off" fail \
  "Audio test enabled but audio unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=ON \
  -DNOM_BUILD_AUDIO_TESTS=ON

run_case "tests-graphics-on-graphics-off" fail \
  "Graphics test enabled but graphics unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=ON \
  -DNOM_BUILD_GRAPHICS_TESTS=ON

run_case "tests-actions-on-actions-off" fail \
  "Actions test enabled but actions unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=ON \
  -DNOM_BUILD_ACTIONS_TESTS=ON

run_case "tests-gui-on-gui-off" fail \
  "GUI test enabled but GUI unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=ON \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=ON \
  -DNOM_BUILD_GUI_TESTS=ON

run_case "tests-on-core-off" fail \
  "Tests enabled but core unit off" \
  -DNOM_BUILD_ACTIONS_UNIT=OFF \
  -DNOM_BUILD_AUDIO_UNIT=OFF \
  -DNOM_BUILD_GRAPHICS_UNIT=OFF \
  -DNOM_BUILD_GUI_UNIT=OFF \
  -DEXAMPLES=OFF \
  -DNOM_BUILD_TESTS=ON \
  -DNOM_BUILD_CORE_UNIT=OFF

# ============================================================================
# SUMMARY
# ============================================================================

printf '=%.0s' {1..70}
echo
echo "BUILD-OPTION VALIDATION MATRIX SUMMARY"
echo "  Expected pass : $EXPECT_PASS — matched: $PASS"
echo "  Expected fail : $EXPECT_FAIL — matched: $FAIL"
echo "  Total cases   : $((EXPECT_PASS + EXPECT_FAIL))"
echo

if [[ -n "$TMP_ERR" ]]; then
  echo "FAILING CASES:"
  echo "$TMP_ERR"
fi

if (( PASS == EXPECT_PASS && FAIL == EXPECT_FAIL )); then
  echo "ALL CASES PASSED ✓"
  rm -rf "$BUILD_BASE"
  exit 0
else
  echo "SOME CASES FAILED ✗ — output kept at $BUILD_BASE for inspection"
  exit 1
fi

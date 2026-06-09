#!/bin/bash
#
# bin/check_dev_entrypoints.sh:
#   Regression test for unified dev entrypoints.
#   Verifies that bin/ and .vscode/bin/ configure scripts produce identical
#   CMakeCache values, and that flags / legacy args / cache clear all work.
#
# Usage:
#   bash bin/check_dev_entrypoints.sh
#
# The script creates a temp directory, runs cmake several times, and cleans up.
# It does NOT touch the project source tree or any existing build dir.
#

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PASS=0
FAIL=0

ok()   { PASS=$((PASS+1)); echo "  PASS  $1"; }
fail() { FAIL=$((FAIL+1)); echo "  FAIL  $1"; }

read_cache() {
    # $1 = build dir
    # prints lines: KEY=VALUE (for keys we care about)
    local cache="$1/CMakeCache.txt"
    [[ -f "$cache" ]] || return 1
    # Strip comments, extract KEY:TYPE=VALUE → KEY=VALUE
    sed -e '/^#/d' -e '/^\/\//d' -e '/^$/d' \
        -e 's/^\([^:]*\):[^=]*=/\1=/' "$cache"
}

cache_get() {
    # $1 = build dir, $2 = key
    local val
    val="$(read_cache "$1" | awk -F= -v k="$2" '$1==k{sub(/^[^=]*=/,"");print;exit}')"
    echo "${val:-<missing>}"
}

compare_caches() {
    # $1 = label, $2 = dir_bin, $3 = dir_vsc, $4.. = keys to compare
    local label="$1"; shift
    local dir_bin="$1"; shift
    local dir_vsc="$1"; shift
    local all_ok=1 k va vb
    for k in "$@"; do
        va="$(cache_get "$dir_bin" "$k")"
        vb="$(cache_get "$dir_vsc" "$k")"
        if [[ "$va" != "$vb" ]]; then
            fail "$label: CMakeCache $k differs: bin/='$va' vs .vscode/bin/='$vb'"
            all_ok=0
        fi
    done
    if [[ $all_ok -eq 1 ]]; then
        ok "$label: all $# cache keys match between bin/ and .vscode/bin/"
    fi
}

check_cache_values() {
    # $1 = label, $2 = build dir, rest are KEY=VALUE expectations
    local label="$1"; shift
    local dir="$1"; shift
    local all_ok=1 k ev av
    for spec in "$@"; do
        k="${spec%%=*}"
        ev="${spec#*=}"
        av="$(cache_get "$dir" "$k")"
        if [[ "$av" != "$ev" ]]; then
            fail "$label: expected $k='$ev', got '$av'"
            all_ok=0
        fi
    done
    if [[ $all_ok -eq 1 ]]; then
        ok "$label: all expected values landed in CMakeCache"
    fi
}

TMP_ROOT="$(mktemp -d -t nomlib-dev-entrypoints)"
trap 'rm -rf "$TMP_ROOT"' EXIT

KEYS_COMPARE=(
    CMAKE_BUILD_TYPE CMAKE_INSTALL_PREFIX
    DEBUG DEBUG_ASSERT
    EXAMPLES NOM_BUILD_TESTS DOCS
)

# ---------------------------------------------------------------------------
echo "======================================================================"
echo "TEST 1: bin/configure.sh vs .vscode/bin/configure.sh identical cache"
echo "======================================================================"

run_combo() {
    # $1 = combo name, $2 = build dir, $3 = entrypoint script, rest = args
    local name="$1"; shift
    local dir="$1"; shift
    local entry="$1"; shift
    mkdir -p "$dir"
    bash "$entry" --build-dir "$dir" "$@" >/dev/null 2>&1 || true
}

# combo: 1 Debug defaults
DIR1_BIN="${TMP_ROOT}/debug_defaults_bin"
DIR1_VSC="${TMP_ROOT}/debug_defaults_vsc"
run_combo "debug bin"  "$DIR1_BIN" "${PROJECT_ROOT}/bin/configure.sh"             --debug
run_combo "debug vsc"  "$DIR1_VSC" "${PROJECT_ROOT}/.vscode/bin/configure.sh"     --debug
compare_caches "Debug defaults" "$DIR1_BIN" "$DIR1_VSC" "${KEYS_COMPARE[@]}"
check_cache_values "Debug defaults [bin/]"          "$DIR1_BIN" \
    CMAKE_BUILD_TYPE=Debug DEBUG=on DEBUG_ASSERT=on EXAMPLES=on NOM_BUILD_TESTS=on
check_cache_values "Debug defaults [.vscode/bin/]"  "$DIR1_VSC" \
    CMAKE_BUILD_TYPE=Debug DEBUG=on DEBUG_ASSERT=on EXAMPLES=on NOM_BUILD_TESTS=on

# combo: 2 Release defaults
DIR2_BIN="${TMP_ROOT}/release_defaults_bin"
DIR2_VSC="${TMP_ROOT}/release_defaults_vsc"
run_combo "rel bin"  "$DIR2_BIN" "${PROJECT_ROOT}/bin/configure.sh"             --release
run_combo "rel vsc"  "$DIR2_VSC" "${PROJECT_ROOT}/.vscode/bin/configure.sh"     --release
compare_caches "Release defaults" "$DIR2_BIN" "$DIR2_VSC" "${KEYS_COMPARE[@]}"
check_cache_values "Release defaults [bin/]"          "$DIR2_BIN" \
    CMAKE_BUILD_TYPE=Release DEBUG=off DEBUG_ASSERT=off EXAMPLES=on NOM_BUILD_TESTS=on
check_cache_values "Release defaults [.vscode/bin/]"  "$DIR2_VSC" \
    CMAKE_BUILD_TYPE=Release DEBUG=off DEBUG_ASSERT=off EXAMPLES=on NOM_BUILD_TESTS=on

# combo: 3 custom install dir
DIR3_BIN="${TMP_ROOT}/custom_prefix_bin"
DIR3_VSC="${TMP_ROOT}/custom_prefix_vsc"
run_combo "prefix bin"  "$DIR3_BIN" "${PROJECT_ROOT}/bin/configure.sh"             --debug --install-dir /tmp/nomlib-test-prefix
run_combo "prefix vsc"  "$DIR3_VSC" "${PROJECT_ROOT}/.vscode/bin/configure.sh"     --debug --install-dir /tmp/nomlib-test-prefix
compare_caches "Custom install dir" "$DIR3_BIN" "$DIR3_VSC" "${KEYS_COMPARE[@]}"
check_cache_values "Custom install dir [bin/]"          "$DIR3_BIN" \
    CMAKE_BUILD_TYPE=Debug CMAKE_INSTALL_PREFIX=/tmp/nomlib-test-prefix
check_cache_values "Custom install dir [.vscode/bin/]"  "$DIR3_VSC" \
    CMAKE_BUILD_TYPE=Debug CMAKE_INSTALL_PREFIX=/tmp/nomlib-test-prefix

# combo: 4 no tests, no examples
DIR4_BIN="${TMP_ROOT}/no_tests_examples_bin"
DIR4_VSC="${TMP_ROOT}/no_tests_examples_vsc"
run_combo "notests bin"  "$DIR4_BIN" "${PROJECT_ROOT}/bin/configure.sh"             --debug --tests off --examples off
run_combo "notests vsc"  "$DIR4_VSC" "${PROJECT_ROOT}/.vscode/bin/configure.sh"     --debug --tests off --examples off
compare_caches "No tests, no examples" "$DIR4_BIN" "$DIR4_VSC" "${KEYS_COMPARE[@]}"
check_cache_values "No tests, no examples [bin/]"          "$DIR4_BIN" \
    EXAMPLES=off NOM_BUILD_TESTS=off
check_cache_values "No tests, no examples [.vscode/bin/]"  "$DIR4_VSC" \
    EXAMPLES=off NOM_BUILD_TESTS=off

# combo: 5 tests on, examples off + release
DIR5_BIN="${TMP_ROOT}/tests_on_examples_off_bin"
DIR5_VSC="${TMP_ROOT}/tests_on_examples_off_vsc"
run_combo "mixed bin"  "$DIR5_BIN" "${PROJECT_ROOT}/bin/configure.sh"             --release --tests on --examples off
run_combo "mixed vsc"  "$DIR5_VSC" "${PROJECT_ROOT}/.vscode/bin/configure.sh"     --release --tests on --examples off
compare_caches "Tests on, examples off" "$DIR5_BIN" "$DIR5_VSC" "${KEYS_COMPARE[@]}"
check_cache_values "Tests on, examples off [bin/]"          "$DIR5_BIN" \
    CMAKE_BUILD_TYPE=Release EXAMPLES=off NOM_BUILD_TESTS=on
check_cache_values "Tests on, examples off [.vscode/bin/]"  "$DIR5_VSC" \
    CMAKE_BUILD_TYPE=Release EXAMPLES=off NOM_BUILD_TESTS=on

# ---------------------------------------------------------------------------
echo
echo "======================================================================"
echo "TEST 2: Legacy positional args reach CMakeCache (generator w/ spaces)"
echo "======================================================================"

DIR_POS="${TMP_ROOT}/legacy_pos"
bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_POS" \
    "Release" "Unix Makefiles" "/tmp/nomlib-legacy-prefix" >/dev/null 2>&1 || true

check_cache_values "Legacy positional args" "$DIR_POS" \
    CMAKE_BUILD_TYPE=Release \
    CMAKE_INSTALL_PREFIX=/tmp/nomlib-legacy-prefix \
    DEBUG=off DEBUG_ASSERT=off

# ---------------------------------------------------------------------------
echo
echo "======================================================================"
echo "TEST 3: Default cache clear vs --no-cache-clear"
echo "======================================================================"

DIR_CLEAR="${TMP_ROOT}/cache_clear"
mkdir -p "$DIR_CLEAR"
bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_CLEAR" --debug --tests off --examples off >/dev/null 2>&1 || true
echo "STALE_MARKER:STRING=yes_it_is_stale" >> "${DIR_CLEAR}/CMakeCache.txt"
grep -q "^STALE_MARKER=" <(read_cache "$DIR_CLEAR") || { echo "precondition failed"; exit 1; }

bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_CLEAR" --debug >/dev/null 2>&1 || true

if [[ "$(cache_get "$DIR_CLEAR" STALE_MARKER)" == "<missing>" ]]; then
    ok "Default (no flag): CMakeCache was cleared between runs (stale marker removed)"
else
    fail "Default (no flag): CMakeCache was NOT cleared (stale marker still present)"
fi

DIR_NOCLEAR="${TMP_ROOT}/no_cache_clear"
mkdir -p "$DIR_NOCLEAR"
bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_NOCLEAR" --debug --tests off --examples off >/dev/null 2>&1 || true
echo "PERSISTENT_MARKER:STRING=should_stay" >> "${DIR_NOCLEAR}/CMakeCache.txt"

bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_NOCLEAR" --debug --no-cache-clear >/dev/null 2>&1 || true

if [[ "$(cache_get "$DIR_NOCLEAR" PERSISTENT_MARKER)" == "should_stay" ]]; then
    ok "--no-cache-clear: CMakeCache preserved between runs (persistent marker present)"
else
    fail "--no-cache-clear: CMakeCache was cleared despite flag (persistent marker gone)"
fi

# ---------------------------------------------------------------------------
echo
echo "======================================================================"
echo "TEST 4: Legacy positional args do NOT bypass cache clear"
echo "======================================================================"

DIR_LGCLEAR="${TMP_ROOT}/legacy_cache_clear"
mkdir -p "$DIR_LGCLEAR"
bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_LGCLEAR" "Debug" >/dev/null 2>&1 || true
echo "LEGACY_STALE:STRING=remove_me" >> "${DIR_LGCLEAR}/CMakeCache.txt"

bash "${PROJECT_ROOT}/bin/configure.sh" \
    --build-dir "$DIR_LGCLEAR" "Debug" >/dev/null 2>&1 || true

if [[ "$(cache_get "$DIR_LGCLEAR" LEGACY_STALE)" == "<missing>" ]]; then
    ok "Legacy positional: cache still cleared (not bypassed)"
else
    fail "Legacy positional: cache was NOT cleared (bypass bug?)"
fi

# ---------------------------------------------------------------------------
echo
echo "======================================================================"
echo "TEST 5: .vscode/bin/ scripts also clear cache correctly"
echo "======================================================================"

DIR_VSCCLEAR="${TMP_ROOT}/vsc_cache_clear"
mkdir -p "$DIR_VSCCLEAR"
bash "${PROJECT_ROOT}/.vscode/bin/configure.sh" \
    --build-dir "$DIR_VSCCLEAR" --debug --tests off --examples off >/dev/null 2>&1 || true
echo "VSC_STALE:STRING=gone_pls" >> "${DIR_VSCCLEAR}/CMakeCache.txt"

bash "${PROJECT_ROOT}/.vscode/bin/configure.sh" \
    --build-dir "$DIR_VSCCLEAR" --debug >/dev/null 2>&1 || true

if [[ "$(cache_get "$DIR_VSCCLEAR" VSC_STALE)" == "<missing>" ]]; then
    ok ".vscode/bin/configure.sh: cache cleared correctly"
else
    fail ".vscode/bin/configure.sh: cache was NOT cleared"
fi

# ---------------------------------------------------------------------------
echo
echo "======================================================================"
echo "SUMMARY: ${PASS} passed, ${FAIL} failed"
echo "======================================================================"

if [[ $FAIL -eq 0 ]]; then
    echo "All dev entrypoint checks passed."
    exit 0
else
    echo "$FAIL check(s) failed — investigate above."
    exit 1
fi

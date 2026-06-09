
# cmake/preflight.cmake
#
# nomlib Dependency Preflight CMake Integration
#
# This module MUST be included AFTER all project options (NOM_BUILD_*_UNIT,
# NOM_BUILD_TESTS, ...) and third-party path variables (NOMLIB_DEPS_DIR,
# ENV{SDL2DIR}, ...) are declared.
#
# Behavior:
#   * Runs bin/preflight.sh --json
#   * Caches the JSON result to:
#       - CMake variable PREFLIGHT_JSON_RAW (CACHE INTERNAL)
#       - File ${CMAKE_BINARY_DIR}/preflight_result.json (for bin scripts and
#         README tooling to consume)
#   * Parses results and per-check CMake variables
#   * Decides FATAL_ERROR vs. WARNING based on currently enabled modules:
#       - arch_mismatch, env               -> always FATAL_ERROR
#       - gui_missing  + NOM_BUILD_GUI_UNIT   -> FATAL_ERROR
#       - audio_missing + NOM_BUILD_AUDIO_UNIT -> FATAL_ERROR
#       - test_missing  + NOM_BUILD_TESTS      -> FATAL_ERROR
#       - docs_missing                         -> never fatal (optional)
#       - anything else with matching unit OFF -> WARNING only
#
# To skip preflight (e.g. in CI):
#   cmake -DNOM_SKIP_PREFLIGHT=ON ..
# or
#   NOM_SKIP_PREFLIGHT=1 ./bin/configure.sh ...

option(NOM_SKIP_PREFLIGHT "Skip the dependency preflight check" OFF)

if(NOM_SKIP_PREFLIGHT)
  message(STATUS "Preflight: skipped (NOM_SKIP_PREFLIGHT=ON)")
  set(PREFLIGHT_PASSED TRUE CACHE INTERNAL "Preflight overall pass flag")
  set(PREFLIGHT_JSON_RAW "" CACHE INTERNAL "Raw preflight JSON output")
  return()
endif()

set(PREFLIGHT_SCRIPT "${CMAKE_SOURCE_DIR}/bin/preflight.sh")

if(NOT EXISTS "${PREFLIGHT_SCRIPT}")
  message(STATUS "Preflight: script not found at ${PREFLIGHT_SCRIPT}; skipping.")
  set(PREFLIGHT_PASSED TRUE CACHE INTERNAL "Preflight overall pass flag")
  set(PREFLIGHT_JSON_RAW "" CACHE INTERNAL "Raw preflight JSON output")
  return()
endif()

message(STATUS "Running dependency preflight...")

set(PREFLIGHT_ARCH_ARG "")
if(CMAKE_OSX_ARCHITECTURES)
  list(LENGTH CMAKE_OSX_ARCHITECTURES _arch_count)
  if(_arch_count EQUAL 1)
    list(GET CMAKE_OSX_ARCHITECTURES 0 _single_arch)
    set(PREFLIGHT_ARCH_ARG "--arch" "${_single_arch}")
  endif()
endif()

execute_process(
  COMMAND "${PREFLIGHT_SCRIPT}" --json ${PREFLIGHT_ARCH_ARG}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE PREFLIGHT_JSON
  ERROR_VARIABLE PREFLIGHT_STDERR
  RESULT_VARIABLE PREFLIGHT_EXIT_CODE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(PREFLIGHT_STDERR)
  message(STATUS "Preflight stderr: ${PREFLIGHT_STDERR}")
endif()

set(PREFLIGHT_PASSED FALSE)
if(PREFLIGHT_EXIT_CODE EQUAL 0)
  set(PREFLIGHT_PASSED TRUE)
endif()

# ---- Cache JSON result for bin scripts and README ----
set(PREFLIGHT_JSON_RAW "${PREFLIGHT_JSON}" CACHE INTERNAL "Raw preflight JSON output")
if(NOT CMAKE_BINARY_DIR STREQUAL "")
  file(WRITE "${CMAKE_BINARY_DIR}/preflight_result.json" "${PREFLIGHT_JSON}\n")
  message(STATUS "Preflight: result JSON cached at ${CMAKE_BINARY_DIR}/preflight_result.json")
endif()

if(NOT PREFLIGHT_JSON)
  message(STATUS "Preflight: no JSON output; continuing without validation.")
  return()
endif()

string(JSON PREFLIGHT_SUMMARY GET "${PREFLIGHT_JSON}" "summary")
string(JSON PREFLIGHT_TOTAL GET "${PREFLIGHT_SUMMARY}" "total")
string(JSON PREFLIGHT_PASS_COUNT GET "${PREFLIGHT_SUMMARY}" "passed")
string(JSON PREFLIGHT_FAIL_COUNT GET "${PREFLIGHT_SUMMARY}" "failed")
string(JSON PREFLIGHT_WARN_COUNT GET "${PREFLIGHT_SUMMARY}" "warnings")

string(JSON PREFLIGHT_TARGET_ARCH GET "${PREFLIGHT_JSON}" "target_arch")
string(JSON PREFLIGHT_HOST_ARCH GET "${PREFLIGHT_JSON}" "host_arch")
string(JSON PREFLIGHT_PLATFORM GET "${PREFLIGHT_JSON}" "platform")

message(STATUS
  "Preflight: ${PREFLIGHT_PASS_COUNT} passed, ${PREFLIGHT_FAIL_COUNT} failed, "
  "${PREFLIGHT_WARN_COUNT} warnings (target=${PREFLIGHT_TARGET_ARCH}, host=${PREFLIGHT_HOST_ARCH})"
)

string(JSON PREFLIGHT_CAT_ARRAY GET "${PREFLIGHT_JSON}" "failed_categories")
string(JSON PREFLIGHT_CAT_LEN LENGTH "${PREFLIGHT_CAT_ARRAY}")
set(PREFLIGHT_FAILED_CATEGORIES "")
if(PREFLIGHT_CAT_LEN GREATER 0)
  math(EXPR PREFLIGHT_CAT_LAST "${PREFLIGHT_CAT_LEN} - 1")
  foreach(_cat_idx RANGE 0 ${PREFLIGHT_CAT_LAST})
    string(JSON _cat GET "${PREFLIGHT_CAT_ARRAY}" ${_cat_idx})
    list(APPEND PREFLIGHT_FAILED_CATEGORIES "${_cat}")
  endforeach()
endif()

string(JSON PREFLIGHT_CHECKS_ARRAY GET "${PREFLIGHT_JSON}" "checks")
string(JSON PREFLIGHT_CHECKS_LEN LENGTH "${PREFLIGHT_CHECKS_ARRAY}")
set(PREFLIGHT_CHECKS "")

if(PREFLIGHT_CHECKS_LEN GREATER 0)
  math(EXPR PREFLIGHT_CHECKS_LAST "${PREFLIGHT_CHECKS_LEN} - 1")
  foreach(_idx RANGE 0 ${PREFLIGHT_CHECKS_LAST})
    string(JSON _check GET "${PREFLIGHT_CHECKS_ARRAY}" ${_idx})
    string(JSON _name GET "${_check}" "name")
    string(JSON _status GET "${_check}" "status")
    string(JSON _category GET "${_check}" "category")

    string(JSON _detail_type TYPE "${_check}" "detail")
    set(_detail "")
    if(NOT _detail_type STREQUAL "null")
      string(JSON _detail GET "${_check}" "detail")
    endif()

    string(JSON _fix_type TYPE "${_check}" "fix")
    set(_fix "")
    if(NOT _fix_type STREQUAL "null")
      string(JSON _fix GET "${_check}" "fix")
    endif()

    string(MAKE_C_IDENTIFIER "${_name}" _name_id)
    string(TOUPPER "${_name_id}" _name_upper)

    list(APPEND PREFLIGHT_CHECKS "${_name}")
    set("PREFLIGHT_${_name_upper}_STATUS"   "${_status}"   CACHE INTERNAL "Preflight status for ${_name}")
    set("PREFLIGHT_${_name_upper}_CATEGORY" "${_category}" CACHE INTERNAL "Preflight category for ${_name}")
    set("PREFLIGHT_${_name_upper}_DETAIL"   "${_detail}"   CACHE INTERNAL "Preflight detail for ${_name}")
    set("PREFLIGHT_${_name_upper}_FIX"      "${_fix}"      CACHE INTERNAL "Preflight fix for ${_name}")
  endforeach()
endif()

# ---------------------------------------------------------------------------
# Enforce: build the failure report and decide FATAL vs. WARNING per module
# ---------------------------------------------------------------------------

if(NOT PREFLIGHT_PASSED)

  set(_fail_report "")
  set(_fatal_report "")
  set(_warn_report "")
  set(_should_fatal FALSE)

  # Collect enabled-module status for decision-making
  set(_gui_enabled   FALSE)
  set(_audio_enabled FALSE)
  set(_tests_enabled FALSE)
  if(DEFINED NOM_BUILD_GUI_UNIT AND NOM_BUILD_GUI_UNIT)
    set(_gui_enabled TRUE)
  endif()
  if(DEFINED NOM_BUILD_AUDIO_UNIT AND NOM_BUILD_AUDIO_UNIT)
    set(_audio_enabled TRUE)
  endif()
  if(DEFINED NOM_BUILD_TESTS AND NOM_BUILD_TESTS)
    set(_tests_enabled TRUE)
  endif()

  if(PREFLIGHT_CHECKS_LEN GREATER 0)
    math(EXPR PREFLIGHT_CHECKS_LAST "${PREFLIGHT_CHECKS_LEN} - 1")
    foreach(_idx RANGE 0 ${PREFLIGHT_CHECKS_LAST})
      string(JSON _check GET "${PREFLIGHT_CHECKS_ARRAY}" ${_idx})
      string(JSON _name GET "${_check}" "name")
      string(JSON _status GET "${_check}" "status")

      if(_status STREQUAL "FAIL")
        string(JSON _category GET "${_check}" "category")

        string(JSON _detail_type TYPE "${_check}" "detail")
        set(_detail_text "")
        if(NOT _detail_type STREQUAL "null")
          string(JSON _detail_text GET "${_check}" "detail")
        endif()

        string(JSON _fix_type TYPE "${_check}" "fix")
        set(_fix_text "")
        if(NOT _fix_type STREQUAL "null")
          string(JSON _fix_text GET "${_check}" "fix")
        endif()

        set(_is_fatal FALSE)
        set(_skip_reason "")

        # ---- Decision logic per category ----
        if(_category STREQUAL "arch_mismatch")
          set(_is_fatal TRUE)
        elseif(_category STREQUAL "env")
          set(_is_fatal TRUE)
        elseif(_category STREQUAL "gui_missing")
          if(_gui_enabled)
            set(_is_fatal TRUE)
          else()
            set(_skip_reason "NOM_BUILD_GUI_UNIT=OFF")
          endif()
        elseif(_category STREQUAL "audio_missing")
          if(_audio_enabled)
            set(_is_fatal TRUE)
          else()
            set(_skip_reason "NOM_BUILD_AUDIO_UNIT=OFF")
          endif()
        elseif(_category STREQUAL "test_missing")
          if(_tests_enabled)
            set(_is_fatal TRUE)
          else()
            set(_skip_reason "NOM_BUILD_TESTS=OFF")
          endif()
        elseif(_category STREQUAL "docs_missing")
          set(_skip_reason "optional docs tooling")
        else()
          set(_is_fatal TRUE)
        endif()

        # Build the per-check display block
        set(_block "    [${_status}] ${_name} (${_category})")
        if(_skip_reason)
          string(APPEND _block "  [SKIPPED: ${_skip_reason}]")
        endif()
        string(APPEND _block "\n")
        if(_detail_text)
          string(REPLACE "\n" "\n           " _detail_indented "${_detail_text}")
          string(APPEND _block "           ${_detail_indented}\n")
        endif()
        if(_fix_text)
          string(REPLACE "\n" "\n           " _fix_indented "${_fix_text}")
          string(APPEND _block "           Fix: ${_fix_indented}\n")
        endif()
        string(APPEND _block "\n")

        if(_is_fatal)
          string(APPEND _fatal_report "${_block}")
          set(_should_fatal TRUE)
        else()
          string(APPEND _warn_report "${_block}")
        endif()
      endif()
    endforeach()
  endif()

  # Header
  set(_header
    "======================================================================\n"
    "  nomlib PREFLIGHT -- ISSUES DETECTED\n"
    "======================================================================\n"
    "\n"
    "  Enabled modules in this configuration:\n"
    "    NOM_BUILD_GUI_UNIT   = ${_gui_enabled}\n"
    "    NOM_BUILD_AUDIO_UNIT = ${_audio_enabled}\n"
    "    NOM_BUILD_TESTS      = ${_tests_enabled}\n"
    "\n"
  )

  set(_footer
    "  Manual preflight:\n"
    "    ${PREFLIGHT_SCRIPT}\n"
    "  View only fixes:\n"
    "    ${PREFLIGHT_SCRIPT} --fixes\n"
    "  Cached JSON result:\n"
    "    ${CMAKE_BINARY_DIR}/preflight_result.json\n"
    "  To skip this check (not recommended):\n"
    "    cmake -DNOM_SKIP_PREFLIGHT=ON ..\n"
    "======================================================================\n"
  )

  if(_should_fatal)
    if(_fatal_report)
      string(PREPEND _fatal_report "  ---- FATAL (blocking build) ----\n\n")
    endif()
    if(_warn_report)
      string(PREPEND _warn_report "  ---- NON-FATAL (skipped due to module OFF) ----\n\n")
    endif()
    message(FATAL_ERROR
      "\n"
      "${_header}"
      "${_fatal_report}"
      "${_warn_report}"
      "${_footer}"
    )
  else()
    if(_warn_report)
      string(PREPEND _warn_report "  ---- NON-FATAL (all failing modules disabled) ----\n\n")
    endif()
    message(WARNING
      "\n"
      "${_header}"
      "${_warn_report}"
      "${_footer}"
    )
  endif()
else()
  message(STATUS "Preflight: all required checks passed.")
endif()

#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OUTPUT_JSON=false
VERBOSE=false
TARGET_ARCH=""
FIXES_ONLY=false

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

nomlib Dependency Preflight Checker

Options:
  --json              Output results as JSON (machine-readable)
  --verbose           Show detailed checking information
  --arch <arch>       Target architecture: arm64 | x86_64 | auto (default: auto)
  --fixes             Show only fixes for failed checks
  -h, --help          Show this help message

Exit codes:
  0   All checks passed
  1   One or more checks failed
  2   Usage error
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --json)
      OUTPUT_JSON=true
      shift
      ;;
    --verbose)
      VERBOSE=true
      shift
      ;;
    --arch)
      if [[ -z "$2" || "$2" == --* ]]; then
        echo "Error: --arch requires an argument" >&2
        exit 2
      fi
      TARGET_ARCH="$2"
      shift 2
      ;;
    --fixes)
      FIXES_ONLY=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ -z "$TARGET_ARCH" || "$TARGET_ARCH" == "auto" ]]; then
  TARGET_ARCH="$(uname -m)"
fi

PASS=0
FAIL=0
WARN=0
RESULT_COUNT=0
declare -a CHECK_NAMES=()
declare -a CHECK_STATUSES=()
declare -a CHECK_CATEGORIES=()
declare -a CHECK_DETAILS=()
declare -a CHECK_FIXES=()
declare -a FAIL_CATEGORIES=()

add_result() {
  local name="$1"
  local status="$2"
  local category="$3"
  local detail="$4"
  local fix="$5"

  CHECK_NAMES[$RESULT_COUNT]="$name"
  CHECK_STATUSES[$RESULT_COUNT]="$status"
  CHECK_CATEGORIES[$RESULT_COUNT]="$category"
  CHECK_DETAILS[$RESULT_COUNT]="$detail"
  CHECK_FIXES[$RESULT_COUNT]="$fix"
  ((RESULT_COUNT++))

  case "$status" in
    PASS) ((PASS++)) ;;
    FAIL)
      ((FAIL++))
      local found=false
      for c in "${FAIL_CATEGORIES[@]}"; do
        if [[ "$c" == "$category" ]]; then found=true; break; fi
      done
      if [[ "$found" == "false" ]]; then
        FAIL_CATEGORIES+=("$category")
      fi
      ;;
    WARN) ((WARN++)) ;;
  esac
}

log_verbose() {
  if [[ "$VERBOSE" == "true" && "$OUTPUT_JSON" == "false" ]]; then
    echo "  [DEBUG] $*"
  fi
}

cmd_exists() {
  command -v "$1" >/dev/null 2>&1
}

detect_host_arch() {
  local arch
  arch="$(uname -m)"
  case "$arch" in
    arm64|aarch64) echo "arm64" ;;
    x86_64|amd64) echo "x86_64" ;;
    i386|i686) echo "x86" ;;
    *) echo "$arch" ;;
  esac
}

get_library_arch() {
  local lib_path="$1"
  if [[ ! -f "$lib_path" ]]; then
    echo ""
    return
  fi
  if cmd_exists file; then
    local file_info
    file_info="$(file "$lib_path" 2>/dev/null || true)"
    if echo "$file_info" | grep -q "arm64\|aarch64"; then
      echo "arm64"
    elif echo "$file_info" | grep -q "x86_64\|x86-64"; then
      echo "x86_64"
    elif echo "$file_info" | grep -q "i386\|i686"; then
      echo "x86"
    else
      echo ""
    fi
  fi
}

detect_platform() {
  case "$(uname -s)" in
    Darwin) echo "osx" ;;
    Linux) echo "linux" ;;
    MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
    *) echo "unknown" ;;
  esac
}

check_third_party_dir() {
  local tp_dir="${PROJECT_ROOT}/third-party"
  local platform
  platform="$(detect_platform)"

  log_verbose "Checking third-party directory: ${tp_dir}"

  local has_content=false
  if [[ -d "$tp_dir" ]]; then
    local item_count
    item_count="$(find "$tp_dir" -mindepth 1 -maxdepth 1 -not -name '.*' | wc -l | tr -d ' ')"
    if [[ "$item_count" -gt 0 ]]; then
      has_content=true
    fi
  fi

  if [[ "$has_content" == "true" ]]; then
    add_result "third-party directory" "PASS" "env" "Found ${item_count} items in third-party/" ""
  else
    local fix=""
    fix+="Download and extract the nomlib ${platform} dependency package from:"
    fix+=$'\n'
    fix+="    http://sourceforge.net/projects/nomlib/files/"
    fix+=$'\n'
    fix+="  into the third-party/ directory. See also: third-party/README.md"
    add_result "third-party directory" "FAIL" "env" "third-party/ directory is empty or missing" "$fix"
  fi
}

check_sdl2() {
  log_verbose "Checking SDL2..."
  local found=false
  local include_dir=""
  local lib_path=""
  local detail=""

  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d "${PROJECT_ROOT}/third-party/osx/SDL2.framework" ]]; then
        found=true
        lib_path="${PROJECT_ROOT}/third-party/osx/SDL2.framework"
        include_dir="${lib_path}/Headers"
        detail="Found SDL2.framework in third-party/osx"
      elif [[ -d ~/Library/Frameworks/SDL2.framework ]]; then
        found=true
        lib_path=~/Library/Frameworks/SDL2.framework
        include_dir="${lib_path}/Headers"
        detail="Found SDL2.framework in ~/Library/Frameworks"
      elif [[ -d /Library/Frameworks/SDL2.framework ]]; then
        found=true
        lib_path=/Library/Frameworks/SDL2.framework
        include_dir="${lib_path}/Headers"
        detail="Found SDL2.framework in /Library/Frameworks"
      fi
      ;;
    linux)
      if cmd_exists pkg-config && pkg-config --exists sdl2 2>/dev/null; then
        found=true
        include_dir="$(pkg-config --cflags-only-I sdl2 2>/dev/null || true)"
        lib_path="$(pkg-config --libs sdl2 2>/dev/null || true)"
        detail="Found SDL2 via pkg-config"
      fi
      ;;
  esac

  if cmd_exists sdl2-config 2>/dev/null; then
    found=true
    detail="Found SDL2 via sdl2-config"
  fi

  if [[ "$found" == "true" ]]; then
    local lib_arch=""
    if [[ -n "$lib_path" && -d "$lib_path" ]]; then
      local binary="${lib_path}/SDL2"
      if [[ -f "$binary" ]]; then
        lib_arch="$(get_library_arch "$binary")"
      fi
    fi
    if [[ -n "$lib_arch" && "$lib_arch" != "$TARGET_ARCH" ]]; then
      local fix=""
      fix+="SDL2 is built for ${lib_arch} but target is ${TARGET_ARCH}."
      fix+=$'\n'
      fix+="  Rebuild or reinstall SDL2 for the correct architecture."
      fix+=$'\n'
      fix+="  On macOS Apple Silicon, ensure you use native arm64 Homebrew (/opt/homebrew) not Rosetta x86_64 (/usr/local)."
      add_result "SDL2" "FAIL" "arch_mismatch" "Architecture mismatch: library=${lib_arch}, target=${TARGET_ARCH}" "$fix"
    else
      add_result "SDL2" "PASS" "gui" "$detail" ""
    fi
  else
    local fix=""
    fix+="Install SDL2:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install sdl2"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install libsdl2"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libsdl2-dev"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install SDL2-devel"
    fix+=$'\n'
    fix+="  Or download from:  http://libsdl.org/"
    add_result "SDL2" "FAIL" "gui_missing" "SDL2 not found" "$fix"
  fi
}

check_sdl2_image() {
  log_verbose "Checking SDL2_image..."
  local found=false
  local detail=""
  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d "${PROJECT_ROOT}/third-party/osx/SDL2_image.framework" ]]; then
        found=true
        detail="Found SDL2_image.framework in third-party/osx"
      elif [[ -d ~/Library/Frameworks/SDL2_image.framework ]]; then
        found=true
        detail="Found SDL2_image.framework in ~/Library/Frameworks"
      elif [[ -d /Library/Frameworks/SDL2_image.framework ]]; then
        found=true
        detail="Found SDL2_image.framework in /Library/Frameworks"
      fi
      ;;
    linux)
      if cmd_exists pkg-config && pkg-config --exists SDL2_image 2>/dev/null; then
        found=true
        detail="Found SDL2_image via pkg-config"
      fi
      ;;
  esac

  if [[ "$found" == "true" ]]; then
    add_result "SDL2_image" "PASS" "gui" "$detail" ""
  else
    local fix=""
    fix+="Install SDL2_image:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install sdl2_image"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install libsdl2_image"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libsdl2-image-dev"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install SDL2_image-devel"
    fix+=$'\n'
    fix+="  Or download from:  http://www.libsdl.org/projects/SDL_image/"
    add_result "SDL2_image" "FAIL" "gui_missing" "SDL2_image not found" "$fix"
  fi
}

check_sdl2_ttf() {
  log_verbose "Checking SDL2_ttf..."
  local found=false
  local detail=""
  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d "${PROJECT_ROOT}/third-party/osx/SDL2_ttf.framework" ]]; then
        found=true
        detail="Found SDL2_ttf.framework in third-party/osx"
      elif [[ -d ~/Library/Frameworks/SDL2_ttf.framework ]]; then
        found=true
        detail="Found SDL2_ttf.framework in ~/Library/Frameworks"
      elif [[ -d /Library/Frameworks/SDL2_ttf.framework ]]; then
        found=true
        detail="Found SDL2_ttf.framework in /Library/Frameworks"
      fi
      ;;
    linux)
      if cmd_exists pkg-config && pkg-config --exists SDL2_ttf 2>/dev/null; then
        found=true
        detail="Found SDL2_ttf via pkg-config"
      fi
      ;;
  esac

  if [[ "$found" == "true" ]]; then
    add_result "SDL2_ttf" "PASS" "gui" "$detail" ""
  else
    local fix=""
    fix+="Install SDL2_ttf:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install sdl2_ttf"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install libsdl2_ttf"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libsdl2-ttf-dev"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install SDL2_ttf-devel"
    fix+=$'\n'
    fix+="  Or download from:  http://www.libsdl.org/projects/SDL_ttf/"
    add_result "SDL2_ttf" "FAIL" "gui_missing" "SDL2_ttf not found" "$fix"
  fi
}

check_openal() {
  log_verbose "Checking OpenAL..."
  local found=false
  local detail=""
  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d /System/Library/Frameworks/OpenAL.framework ]] || \
         [[ -d /Library/Frameworks/OpenAL.framework ]]; then
        found=true
        detail="Found OpenAL.framework (system)"
      fi
      ;;
    linux)
      if cmd_exists pkg-config && pkg-config --exists openal 2>/dev/null; then
        found=true
        detail="Found OpenAL via pkg-config"
      elif [[ -f /usr/include/AL/al.h ]]; then
        found=true
        detail="Found OpenAL headers at /usr/include/AL"
      fi
      ;;
  esac

  if [[ "$found" == "true" ]]; then
    add_result "OpenAL" "PASS" "audio" "$detail" ""
  else
    local fix=""
    fix+="Install OpenAL:"
    fix+=$'\n'
    fix+="  macOS: OpenAL is included with the system (should be present)."
    fix+=$'\n'
    fix+="    If missing, install Xcode Command Line Tools: xcode-select --install"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libopenal-dev"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install openal-soft-devel"
    fix+=$'\n'
    fix+="  Or download from:  http://kcat.strangesoft.net/openal.html"
    add_result "OpenAL" "FAIL" "audio_missing" "OpenAL not found" "$fix"
  fi
}

check_libsndfile() {
  log_verbose "Checking libsndfile..."
  local found=false
  local detail=""
  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d "${PROJECT_ROOT}/third-party/osx/sndfile.framework" ]] || \
         [[ -d "${PROJECT_ROOT}/third-party/osx/libsndfile.framework" ]]; then
        found=true
        detail="Found libsndfile framework in third-party/osx"
      fi
      if cmd_exists pkg-config && pkg-config --exists sndfile 2>/dev/null; then
        found=true
        detail="Found libsndfile via pkg-config"
      fi
      ;;
    linux)
      if cmd_exists pkg-config && pkg-config --exists sndfile 2>/dev/null; then
        found=true
        detail="Found libsndfile via pkg-config"
      fi
      ;;
  esac

  if cmd_exists sndfile-info 2>/dev/null; then
    found=true
    detail="Found libsndfile via sndfile-info binary"
  fi

  if [[ "$found" == "true" ]]; then
    add_result "libsndfile" "PASS" "audio" "$detail" ""
  else
    local fix=""
    fix+="Install libsndfile:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install libsndfile"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install libsndfile"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libsndfile1-dev"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install libsndfile-devel"
    fix+=$'\n'
    fix+="  Or download from:  http://www.mega-nerd.com/libsndfile/"
    add_result "libsndfile" "FAIL" "audio_missing" "libsndfile not found" "$fix"
  fi
}

check_librocket() {
  log_verbose "Checking LibRocket..."
  local found=false
  local detail=""
  local platform
  platform="$(detect_platform)"

  case "$platform" in
    osx)
      if [[ -d "${PROJECT_ROOT}/third-party/osx/librocket" ]]; then
        local rocket_lib="${PROJECT_ROOT}/third-party/osx/librocket/lib/libRocketCore.dylib"
        local rocket_header="${PROJECT_ROOT}/third-party/osx/librocket/include/Rocket/Core"
        if [[ -f "$rocket_lib" || -d "$rocket_header" ]]; then
          found=true
          detail="Found LibRocket in third-party/osx/librocket"
        fi
      fi
      if [[ -d /usr/local/include/Rocket/Core ]] || [[ -d /opt/homebrew/include/Rocket/Core ]]; then
        found=true
        detail="Found LibRocket headers in system include path"
      fi
      ;;
    linux)
      if [[ -d /usr/include/Rocket/Core ]] || [[ -d /usr/local/include/Rocket/Core ]]; then
        found=true
        detail="Found LibRocket headers in system include path"
      fi
      ;;
  esac

  if [[ "$found" == "true" ]]; then
    add_result "LibRocket" "PASS" "gui" "$detail" ""
  else
    local fix=""
    fix+="Install LibRocket:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install librocket  # or build from source"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install librocket-dev  # if available"
    fix+=$'\n'
    fix+="  Source build:"
    fix+=$'\n'
    fix+="    git clone https://github.com/libRocket/libRocket.git"
    fix+=$'\n'
    fix+="    cd libRocket && cmake -DBUILD_SAMPLES=off .. && make && sudo make install"
    fix+=$'\n'
    fix+="  Or use the pre-packaged version from: http://sourceforge.net/projects/nomlib/files/"
    add_result "LibRocket" "FAIL" "gui_missing" "LibRocket not found" "$fix"
  fi
}

check_gtest() {
  log_verbose "Checking GTest (Google Test)..."
  local found=false
  local detail=""

  if [[ -d "${PROJECT_ROOT}/third-party/linux/gtest/include" ]] || \
     [[ -d "${PROJECT_ROOT}/third-party/osx/gtest/include" ]] || \
     [[ -d "${PROJECT_ROOT}/third-party/windows/gtest/include" ]]; then
    found=true
    detail="Found GTest in third-party/"
  fi

  local include_paths=(/usr/include /usr/local/include /opt/homebrew/include /opt/local/include)
  for ip in "${include_paths[@]}"; do
    if [[ -d "${ip}/gtest" ]]; then
      found=true
      detail="Found GTest headers at ${ip}/gtest"
      break
    fi
  done

  if cmd_exists pkg-config && pkg-config --exists gtest 2>/dev/null; then
    found=true
    detail="Found GTest via pkg-config"
  fi

  if [[ -f /usr/src/gtest/src/gtest.cc ]] || [[ -f /usr/src/googletest/googletest/src/gtest.cc ]]; then
    found=true
    detail="Found GTest source (needs to be built)"
  fi

  if [[ "$found" == "true" ]]; then
    add_result "GTest" "PASS" "test" "$detail" ""
  else
    local fix=""
    fix+="Install Google Test:"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install googletest"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install libgtest-dev"
    fix+=$'\n'
    fix+="    Then build it:"
    fix+=$'\n'
    fix+="      cd /usr/src/gtest && sudo cmake . && sudo make"
    fix+=$'\n'
    fix+="      sudo cp libgtest*.a /usr/lib"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install gtest-devel"
    fix+=$'\n'
    fix+="  Source: https://github.com/google/googletest"
    add_result "GTest" "FAIL" "test_missing" "GTest (Google Test) not found" "$fix"
  fi
}

check_doxygen() {
  log_verbose "Checking Doxygen..."
  if cmd_exists doxygen; then
    local version
    version="$(doxygen --version 2>/dev/null || echo "unknown")"
    add_result "Doxygen" "PASS" "docs" "Found Doxygen ${version}" ""
  else
    local fix=""
    fix+="Install Doxygen (optional, only needed for generating API docs):"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install doxygen"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install doxygen"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install doxygen"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install doxygen"
    fix+=$'\n'
    fix+="  Or download from:  http://www.doxygen.org/"
    add_result "Doxygen" "WARN" "docs_missing" "Doxygen not found (optional for docs)" "$fix"
  fi
}

check_graphviz() {
  log_verbose "Checking Graphviz..."
  if cmd_exists dot; then
    local version
    version="$(dot -V 2>&1 || echo "unknown")"
    add_result "Graphviz" "PASS" "docs" "Found ${version}" ""
  else
    local fix=""
    fix+="Install Graphviz (optional, needed for Doxygen graphs):"
    fix+=$'\n'
    fix+="  macOS (Homebrew):  brew install graphviz"
    fix+=$'\n'
    fix+="  macOS (MacPorts):  sudo port install graphviz"
    fix+=$'\n'
    fix+="  Linux (Debian/Ubuntu):  sudo apt-get install graphviz"
    fix+=$'\n'
    fix+="  Linux (Fedora):        sudo dnf install graphviz"
    fix+=$'\n'
    fix+="  Or download from:  http://www.graphviz.org/"
    add_result "Graphviz" "WARN" "docs_missing" "Graphviz not found (optional for docs)" "$fix"
  fi
}

check_architecture() {
  log_verbose "Checking target architecture: ${TARGET_ARCH}"
  local host_arch
  host_arch="$(detect_host_arch)"
  local platform
  platform="$(detect_platform)"

  add_result "Target architecture" "PASS" "env" "Target: ${TARGET_ARCH}, Host: ${host_arch}, Platform: ${platform}" ""

  if [[ "$platform" == "osx" ]]; then
    local rosetta=false
    if [[ "$host_arch" == "arm64" && "$TARGET_ARCH" == "x86_64" ]]; then
      if [[ -f /usr/libexec/rosetta/oahd ]] || /usr/sbin/sysctl -n sysctl.proc_translated 2>/dev/null | grep -q 1; then
        rosetta=true
      fi
      local fix=""
      fix+="Building for x86_64 on Apple Silicon (arm64) via Rosetta 2."
      fix+=$'\n'
      fix+="  For native performance, build for arm64 instead."
      fix+=$'\n'
      fix+="  Make sure all dependencies are available for the target architecture."
      if [[ "$rosetta" == "true" ]]; then
        add_result "Architecture cross-build" "WARN" "arch_mismatch" "Building x86_64 on arm64 host (Rosetta detected)" "$fix"
      else
        add_result "Architecture cross-build" "WARN" "arch_mismatch" "Building x86_64 on arm64 host (Rosetta not confirmed)" "$fix"
      fi
    elif [[ "$host_arch" == "x86_64" && "$TARGET_ARCH" == "arm64" ]]; then
      local fix=""
      fix+="Cannot build arm64 binaries on an x86_64 host."
      fix+=$'\n'
      fix+="  Build on an Apple Silicon Mac, or change target to x86_64."
      add_result "Architecture cross-build" "FAIL" "arch_mismatch" "Cannot cross-compile arm64 on x86_64 host" "$fix"
    fi
  fi

  local homebrew_prefix=""
  if [[ "$TARGET_ARCH" == "arm64" ]]; then
    homebrew_prefix="/opt/homebrew"
  elif [[ "$TARGET_ARCH" == "x86_64" ]]; then
    homebrew_prefix="/usr/local"
  fi

  if [[ -n "$homebrew_prefix" && -d "$homebrew_prefix" ]]; then
    add_result "Homebrew prefix" "PASS" "env" "Homebrew prefix for ${TARGET_ARCH}: ${homebrew_prefix}" ""
  elif [[ "$platform" == "osx" ]]; then
    local fix=""
    fix+="Expected Homebrew prefix for ${TARGET_ARCH} not found at ${homebrew_prefix}."
    fix+=$'\n'
    fix+="  arm64 should use:  /opt/homebrew"
    fix+=$'\n'
    fix+="  x86_64 should use: /usr/local"
    fix+=$'\n'
    fix+="  Install Homebrew:   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    add_result "Homebrew prefix" "WARN" "env" "Homebrew prefix not detected for ${TARGET_ARCH}" "$fix"
  fi
}

print_indented() {
  local text="$1"
  local indent="$2"
  while IFS= read -r line; do
    echo "${indent}${line}"
  done <<< "$text"
}

print_human_readable() {
  local red green yellow reset bold
  if [[ -t 1 ]]; then
    red=$'\033[31m'
    green=$'\033[32m'
    yellow=$'\033[33m'
    bold=$'\033[1m'
    reset=$'\033[0m'
  else
    red="" green="" yellow="" bold="" reset=""
  fi

  local total=$((PASS + FAIL + WARN))

  echo ""
  echo "${bold}=== nomlib Dependency Preflight ===${reset}"
  echo "  Target Architecture: ${TARGET_ARCH}"
  echo "  Host Platform:       $(detect_platform) ($(uname -s))"
  echo ""

  if [[ "$FIXES_ONLY" == "false" ]]; then
    local i=0
    while [[ $i -lt $RESULT_COUNT ]]; do
      local name="${CHECK_NAMES[$i]}"
      local status="${CHECK_STATUSES[$i]}"
      local detail="${CHECK_DETAILS[$i]}"
      local status_color status_label
      case "$status" in
        PASS) status_color="$green"; status_label="✓ PASS" ;;
        FAIL) status_color="$red";   status_label="✗ FAIL" ;;
        WARN) status_color="$yellow"; status_label="⚠ WARN" ;;
      esac
      echo "  ${status_color}${status_label}${reset}  ${name}"
      if [[ -n "$detail" && "$VERBOSE" == "true" ]]; then
        print_indented "$detail" "          "
      fi
      ((i++))
    done
    echo ""
  fi

  local shown_fix_header=false
  local i=0
  while [[ $i -lt $RESULT_COUNT ]]; do
    local name="${CHECK_NAMES[$i]}"
    local status="${CHECK_STATUSES[$i]}"
    local detail="${CHECK_DETAILS[$i]}"
    local fix="${CHECK_FIXES[$i]}"
    if [[ "$status" == "FAIL" || ( "$status" == "WARN" && "$FIXES_ONLY" == "true" ) ]]; then
      if [[ "$shown_fix_header" == "false" ]]; then
        echo "${bold}${red}=== Required Fixes ===${reset}"
        echo ""
        shown_fix_header=true
      fi
      local label="FIX"
      if [[ "$status" == "WARN" ]]; then label="SUGGESTION"; fi
      echo "  ${bold}[${label}] ${name}:${reset}"
      if [[ -n "$detail" ]]; then
        print_indented "$detail" "    "
      fi
      if [[ -n "$fix" ]]; then
        print_indented "$fix" "    "
      fi
      echo ""
    fi
    ((i++))
  done

  echo "${bold}=== Summary ===${reset}"
  echo "  ${green}${PASS} passed${reset}, ${red}${FAIL} failed${reset}, ${yellow}${WARN} warnings${reset} (${total} total checks)"
  echo ""

  if [[ $FAIL -gt 0 ]]; then
    echo "${bold}${red}Preflight FAILED. Please fix the issues above before building.${reset}"
  else
    echo "${bold}${green}Preflight PASSED. Ready to build!${reset}"
    if [[ $WARN -gt 0 ]]; then
      echo "  ${yellow}(Some optional dependencies are missing — see above)${reset}"
    fi
  fi
  echo ""
}

json_escape() {
  local s="$1"
  if cmd_exists python3; then
    printf '%s' "$s" | python3 -c 'import sys,json; print(json.dumps(sys.stdin.read()), end="")'
  else
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    s="${s//$'\n'/\\n}"
    s="${s//$'\r'/\\r}"
    s="${s//$'\t'/\\t}"
    printf '"%s"' "$s"
  fi
}

print_json() {
  local platform
  platform="$(detect_platform)"
  local host_arch
  host_arch="$(detect_host_arch)"

  printf '{\n'
  printf '  "target_arch": %s,\n' "$(json_escape "$TARGET_ARCH")"
  printf '  "host_arch": %s,\n' "$(json_escape "$host_arch")"
  printf '  "platform": %s,\n' "$(json_escape "$platform")"
  printf '  "summary": {\n'
  printf '    "passed": %d,\n' "$PASS"
  printf '    "failed": %d,\n' "$FAIL"
  printf '    "warnings": %d,\n' "$WARN"
  printf '    "total": %d\n' "$((PASS + FAIL + WARN))"
  printf '  },\n'

  printf '  "failed_categories": ['
  for i in "${!FAIL_CATEGORIES[@]}"; do
    if [[ $i -gt 0 ]]; then printf ', '; fi
    printf '%s' "$(json_escape "${FAIL_CATEGORIES[$i]}")"
  done
  printf '],\n'

  printf '  "checks": [\n'
  local i=0
  while [[ $i -lt $RESULT_COUNT ]]; do
    if [[ $i -gt 0 ]]; then printf ',\n'; fi
    local name="${CHECK_NAMES[$i]}"
    local status="${CHECK_STATUSES[$i]}"
    local category="${CHECK_CATEGORIES[$i]}"
    local detail="${CHECK_DETAILS[$i]}"
    local fix="${CHECK_FIXES[$i]}"

    local detail_json="null"
    if [[ -n "$detail" ]]; then
      detail_json="$(json_escape "$detail")"
    fi
    local fix_json="null"
    if [[ -n "$fix" ]]; then
      fix_json="$(json_escape "$fix")"
    fi

    printf '    {\n'
    printf '      "name": %s,\n' "$(json_escape "$name")"
    printf '      "status": %s,\n' "$(json_escape "$status")"
    printf '      "category": %s,\n' "$(json_escape "$category")"
    printf '      "detail": %s,\n' "$detail_json"
    printf '      "fix": %s\n' "$fix_json"
    printf '    }'
    ((i++))
  done
  printf '\n  ]\n'
  printf '}\n'
}

main() {
  check_third_party_dir
  check_architecture
  check_sdl2
  check_sdl2_image
  check_sdl2_ttf
  check_openal
  check_libsndfile
  check_librocket
  check_gtest
  check_doxygen
  check_graphviz

  if [[ "$OUTPUT_JSON" == "true" ]]; then
    print_json
  else
    print_human_readable
  fi

  if [[ $FAIL -gt 0 ]]; then
    exit 1
  else
    exit 0
  fi
}

main "$@"

# QtScrcpy — task runner
#
# Qt client for scrcpy. `just` with no arguments lists every recipe.
# Most recipes take an optional build type: `just build Release`.

set shell := ["bash", "-euo", "pipefail", "-c"]

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Debug | Release | MinSizeRel | RelWithDebInfo
build_type := "Debug"
build_dir  := "build"

# CMake writes binaries to output/<arch>/<build_type>/ (see QtScrcpy/CMakeLists.txt).
# Mirror its arch naming so `just run` can find the binary.
arch := if arch() == "x86_64" { "x64" } else if arch() == "aarch64" { "arm64" } else { "x86" }

# CI sets ENV_QT_PATH (e.g. ~/Qt5.15.2/5.15.2); locally an empty value means
# "use whatever Qt cmake finds on the system", which is the common case.
qt_env := env_var_or_default("ENV_QT_PATH", "")

jobs := num_cpus()

# All translations. The ci/lupdate.sh and ci/lrelease.sh scripts predate ko_KR
# and silently skip it — these recipes cover every .ts file in the tree.
translations := "./QtScrcpy/res/i18n/en_US.ts ./QtScrcpy/res/i18n/zh_CN.ts ./QtScrcpy/res/i18n/ja_JP.ts ./QtScrcpy/res/i18n/ko_KR.ts"

# ---------------------------------------------------------------------------

# List available recipes
default:
    @just --list --unsorted

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------

# Fetch/update the QtScrcpyCore submodule
init:
    git submodule update --init --recursive

# Run CMake configure only
configure bt=build_type:
    @if [ -n "{{qt_env}}" ]; then \
        echo "Using Qt from ENV_QT_PATH: {{qt_env}}"; \
        cmake -S . -B {{build_dir}} \
            -DCMAKE_BUILD_TYPE={{bt}} \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DCMAKE_PREFIX_PATH="{{qt_env}}/gcc_64/lib/cmake/Qt5"; \
    else \
        cmake -S . -B {{build_dir}} \
            -DCMAKE_BUILD_TYPE={{bt}} \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; \
    fi

# Configure (if needed) and compile
build bt=build_type: (configure bt)
    cmake --build {{build_dir}} --config {{bt}} -j{{jobs}}

# Compile without re-running configure — fastest inner loop
b:
    cmake --build {{build_dir}} -j{{jobs}}

# Wipe the build tree and rebuild from scratch
rebuild bt=build_type: clean (build bt)

# Remove build artifacts and the output tree
clean:
    rm -rf {{build_dir}} output

# clean + drop generated translation binaries and the local run log
distclean: clean
    rm -f output.log
    rm -f QtScrcpy/res/i18n/*.qm

# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------

# enable_testing() lives in QtScrcpy/CMakeLists.txt rather than the top-level
# one, so CTest registers the tests under build/QtScrcpy. Pointing ctest at
# build/ instead reports "No tests were found!!!" and exits 0 — a silent pass.
[doc("Run the test suite")]
test: b
    ctest --test-dir {{build_dir}}/QtScrcpy --output-on-failure

# Run the test suite with per-testcase output
test-verbose: b
    ctest --test-dir {{build_dir}}/QtScrcpy --verbose

# Run a single test by name, e.g. `just test-one tst_devicenaming`
test-one name: b
    ctest --test-dir {{build_dir}}/QtScrcpy --output-on-failure -R {{name}}

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------

# .envrc carries DEVICE_CSV_FILE, which the device-naming lookup needs.
[doc("Launch the app, teeing output to output.log")]
run bt=build_type:
    #!/usr/bin/env bash
    set -euo pipefail
    bin="output/{{arch}}/{{bt}}/QtScrcpy"
    if [ ! -x "$bin" ]; then
        echo "error: $bin not found — run 'just build {{bt}}' first" >&2
        exit 1
    fi
    [ -f .envrc ] && source .envrc
    "./$bin" 2>&1 | tee -a output.log

# Follow the run log
log:
    tail -f output.log

# List attached Android devices
devices:
    adb devices -l

# ---------------------------------------------------------------------------
# Code quality
# ---------------------------------------------------------------------------

# Scoped with -prune to skip the QtScrcpyCore submodule and the build tree.
# ./QtScrcpy/clang-format-all.sh recurses blindly, so pointing it at QtScrcpy/
# reformats the submodule's sources too and dirties an unrelated repo.
[doc("Format all C/C++ sources in place")]
fmt:
    #!/usr/bin/env bash
    set -euo pipefail
    command -v clang-format >/dev/null || { echo "error: clang-format not installed" >&2; exit 1; }
    just _sources | xargs -r clang-format -i
    echo "formatted $(just _sources | wc -l) files"

# Report files that differ from .clang-format (non-zero exit if any do)
fmt-check:
    #!/usr/bin/env bash
    set -euo pipefail
    command -v clang-format >/dev/null || { echo "error: clang-format not installed" >&2; exit 1; }
    fail=0
    while read -r f; do
        if ! diff -q "$f" <(clang-format "$f") >/dev/null 2>&1; then
            echo "needs formatting: $f"
            fail=1
        fi
    done < <(just _sources)
    exit $fail

# Emit the list of first-party sources (excludes submodule + build tree)
[private]
_sources:
    @find QtScrcpy \
        -path QtScrcpy/QtScrcpyCore -prune -o \
        -path QtScrcpy/QtScrcpyCore/\* -prune -o \
        \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \
        -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' \) -print

# Symlink compile_commands.json to the repo root for clangd/IDE tooling
cdb: (configure build_type)
    ln -sf {{build_dir}}/compile_commands.json compile_commands.json

# ---------------------------------------------------------------------------
# Translations
# ---------------------------------------------------------------------------

# Rescan sources and update every .ts file
i18n-update:
    lupdate -no-obsolete ./QtScrcpy -ts {{translations}}

# Compile every .ts into the .qm files shipped in the Qt resource bundle
i18n-release:
    lrelease {{translations}}

# ---------------------------------------------------------------------------
# Packaging
# ---------------------------------------------------------------------------

# Write QtScrcpy/appversion from the latest git tag
version:
    python3 ci/generate-version.py

# Build a Linux AppImage
appimage bt="Release": (build bt)
    ./ci/linux/package_appimage.sh {{bt}}

# ---------------------------------------------------------------------------
# Composite
# ---------------------------------------------------------------------------

# What CI should run: build with warnings-as-errors, then the test suite
check bt=build_type: (build bt) test

# Full clean verification from scratch
ci bt="Release": clean (build bt) test

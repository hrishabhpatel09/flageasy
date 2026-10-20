#!/usr/bin/env bash
# Configure, build, and test flageasy into ./build.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$root/build}"

cmake -S "$root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" -j
ctest --test-dir "$build_dir" --output-on-failure

#!/usr/bin/env bash
# End-to-end check: install flageasy into a throwaway prefix, then build and
# run the standalone consumer project against it via find_package(flageasy).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
work_dir="$root/build-install-test"
prefix="$work_dir/prefix"
consumer_build="$work_dir/consumer"

rm -rf "$work_dir"

echo "== building and installing flageasy into $prefix"
cmake -S "$root" -B "$work_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DFLAGEASY_BUILD_TESTS=OFF \
    -DFLAGEASY_BUILD_EXAMPLES=OFF
cmake --build "$work_dir" -j
cmake --install "$work_dir" --prefix "$prefix"

echo "== building consumer against the installed package"
cmake -S "$root/examples/consumer" -B "$consumer_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$consumer_build" -j

echo "== running consumer"
"$consumer_build/consumer" --name FlagEasy
"$consumer_build/consumer" --help >/dev/null

echo "== OK"

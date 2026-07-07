#!/usr/bin/env bash
set -e
# Absolute path to the example directory:
EXAMPLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Figure out the native host target (architecture and kernel);
if [ -z "$TARGET" ] ; then
  ARCH=$(uname -m)
  KERNEL=$(uname -s)
  KERNEL=${KERNEL,,}
  TARGET="${ARCH}-${KERNEL}"
fi

# The build type:
[ -n "$CMAKE_BUILD_TYPE" ] || export CMAKE_BUILD_TYPE=Release

BUILD_DIR="${EXAMPLE_DIR}/build-${TARGET}-${CMAKE_BUILD_TYPE,,}"
cmake -B "$BUILD_DIR" -S "$EXAMPLE_DIR"
cmake --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure

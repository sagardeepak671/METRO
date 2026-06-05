#!/usr/bin/env bash 
set -euo pipefail

echo "compile.sh: starting"

# Clean build if requested
if [ "${CLEAN:-0}" -ne 0 ] 2>/dev/null; then
  echo "Cleaning previous builds..."
  rm -f encoder decoder
fi

# Use g++ compiler (C++11 standard)
CXX=g++

# Two modes: DEBUG=1 for debugging (address/ubsan), else fast-release
if [ "${DEBUG:-0}" -ne 0 ] 2>/dev/null; then
  echo "DEBUG mode: building with sanitizers and debug symbols"
  CXXFLAGS="-std=c++11 -O0 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-optimize-sibling-calls -Wall -Wextra"
  LDFLAGS="-fsanitize=address,undefined"
  STRIP_BINARY=0
else
  echo "RELEASE mode: building for maximum speed (C++11, O3, march=native)"
  # Maximum optimization flags for speed with C++11
  # -O3: highest optimization level
  # -march=native: optimize for current CPU architecture
  # -funroll-loops: unroll loops for speed
  # -ffast-math: fast floating point math (breaks strict IEEE compliance)
  # -DNDEBUG: disable assertions
  # -fomit-frame-pointer: omit frame pointer for more registers
  # -finline-functions: inline functions aggressively
  CXXFLAGS="-std=c++11 -O3 -march=native -funroll-loops -ffast-math -DNDEBUG -Wall -Wextra -fomit-frame-pointer -finline-functions -fno-math-errno -fno-trapping-math"
  LDFLAGS="-O3"
  STRIP_BINARY=1
fi

echo "Using compiler: $CXX"
echo "CXXFLAGS: $CXXFLAGS"

echo "Compiling encoder..."
$CXX $CXXFLAGS encoder.cpp -o encoder $LDFLAGS
echo "Compiling decoder..."
$CXX $CXXFLAGS decoder.cpp -o decoder $LDFLAGS

if [ "$STRIP_BINARY" -ne 0 ] 2>/dev/null; then
  if command -v strip >/dev/null 2>&1; then
    echo "Stripping binaries to reduce size..."
    strip encoder || true
    strip decoder || true
  fi
fi

echo "Build finished: ./encoder, ./decoder"


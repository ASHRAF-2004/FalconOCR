#!/usr/bin/env bash
set -euxo pipefail
cmake -S . -B build -G "${CMAKE_GENERATOR:-Ninja}" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

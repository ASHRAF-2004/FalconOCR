#!/usr/bin/env bash
set -euxo pipefail
# Run clang-tidy using compile_commands.json
if [ ! -f build/compile_commands.json ]; then
  cmake -S . -B build -G "${CMAKE_GENERATOR:-Ninja}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
mapfile -t files < <(find src include -name '*.cpp' -o -name '*.h')
clang-tidy -p build "${files[@]}" --config-file=.codestyle/.clang-tidy

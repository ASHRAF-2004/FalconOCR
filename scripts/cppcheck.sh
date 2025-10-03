#!/usr/bin/env bash
set -euxo pipefail
cppcheck --enable=all --inconclusive --std=c++17 --inline-suppr \
  --suppress=missingIncludeSystem \
  -I include src

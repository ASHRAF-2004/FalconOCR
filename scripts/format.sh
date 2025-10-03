#!/usr/bin/env bash
set -euxo pipefail
find src include tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.codestyle/.clang-format

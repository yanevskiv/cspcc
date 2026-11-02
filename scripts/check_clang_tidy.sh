#!/bin/sh
set -eu

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is required to check the naming contract" >&2
    exit 1
fi

build_dir=build/out
if [ ! -f "$build_dir/compile_commands.json" ]; then
    echo "build the project with ./do_build.sh before running the naming check" >&2
    exit 1
fi

find src tests/unit -type f -name '*.cpp' -print0 |
    xargs -0 -n 1 clang-tidy -p "$build_dir" \
        --header-filter='^.*/include/cspcc/.*' \
        --warnings-as-errors='readability-identifier-naming'

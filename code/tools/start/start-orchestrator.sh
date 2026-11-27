#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
build_dir="$repo_dir/build"

if ! python3 -c 'import matplotlib, tornado'; then
    echo "Install the plot dependencies from code/orchestrator/system/requirements.txt." >&2
    exit 1
fi

cmake -S "$repo_dir" -B "$build_dir"
cmake --build "$build_dir" --target main.out --parallel 4

cd "$build_dir/code/orchestrator/system/bin"
exec ./main.out

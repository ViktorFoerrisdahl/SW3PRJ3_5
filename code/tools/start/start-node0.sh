#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <orchestrator-ip>" >&2
    exit 1
fi

case "$(hostname)" in
    *0) ;;
    *) echo "Node 0 requires a hostname ending in 0 (e.g. atlas-node0)." >&2; exit 1 ;;
esac

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
build_dir="$repo_dir/build"
cmake -S "$repo_dir" -B "$build_dir"
cmake --build "$build_dir" --target atlas_node --parallel 4
exec sudo "$build_dir/code/node/system/atlas_node" "$1"

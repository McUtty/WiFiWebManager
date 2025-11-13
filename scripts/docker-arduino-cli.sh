#!/usr/bin/env bash
set -euo pipefail

IMAGE="arduino/arduino-cli:latest"

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required to run this script" >&2
    exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

exec docker run --rm \
    -v "$PROJECT_ROOT":/workspace \
    -w /workspace \
    "$IMAGE" \
    "$@"

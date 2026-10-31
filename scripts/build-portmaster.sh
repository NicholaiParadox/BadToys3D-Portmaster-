#!/usr/bin/env bash
# Builds bt3d_raylib for PortMaster (aarch64 Linux) inside an arm64 Debian
# container, so the binary links against an old glibc that handheld firmwares
# can run. On an x86 PC this runs under qemu and is slow but works.
#
# Requires: docker (with qemu/binfmt for arm64), run from the repo root.
# Output:   build/portmaster/bt3d_raylib
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${BT3D_PM_IMAGE:-arm64v8/debian:bullseye}"

docker run --rm --platform linux/arm64 \
    -v "$ROOT":/src -w /src "$IMAGE" bash -euc '
        apt-get update
        apt-get install -y --no-install-recommends \
            build-essential cmake git ca-certificates pkg-config \
            libsdl2-dev libasound2-dev libgles2-mesa-dev libdrm-dev libgbm-dev
        cmake -S . -B build/portmaster \
            -DBT3D_PLATFORM_PORTMASTER=ON -DBT3D_FETCH_RAYLIB=ON \
            -DCMAKE_BUILD_TYPE=Release
        cmake --build build/portmaster --parallel "$(nproc)"
        file build/portmaster/bt3d_raylib || true
    '
echo "Done: $ROOT/build/portmaster/bt3d_raylib"

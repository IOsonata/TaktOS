#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if [ -d FreeRTOS-Kernel/.git ]; then
  echo "FreeRTOS-Kernel already present"
  exit 0
fi

rm -rf FreeRTOS-Kernel.tmp

git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git FreeRTOS-Kernel.tmp
mv FreeRTOS-Kernel.tmp FreeRTOS-Kernel

echo "Fetched FreeRTOS-Kernel into: $ROOT_DIR/FreeRTOS-Kernel"

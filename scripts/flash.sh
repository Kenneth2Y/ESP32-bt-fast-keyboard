#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 /dev/cu.usbserial-XXXX"
  exit 2
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
set -a
eval "$(/Users/ky/.espressif/tools/activate_idf_v6.0.1.sh -e)"
set +a
export PATH="$PATH:$SYSTEM_PATH"
cd "$ROOT_DIR"
"$IDF_PATH/tools/idf.py" -p "$1" flash

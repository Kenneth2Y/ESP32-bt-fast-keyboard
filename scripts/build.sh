#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
set -a
eval "$(/Users/ky/.espressif/tools/activate_idf_v6.0.1.sh -e)"
set +a
export PATH="$PATH:$SYSTEM_PATH"
cd "$ROOT_DIR"
"$IDF_PATH/tools/idf.py" build

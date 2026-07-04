#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 /dev/cu.usbserial-XXXX [seconds]"
  exit 2
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="$1"
DURATION="${2:-20}"
STAMP="$(date '+%Y-%m-%d-%H-%M-%S')"
LOG_DIR="$ROOT_DIR/docs/serial-logs"
mkdir -p "$LOG_DIR"

set -a
eval "$(/Users/ky/.espressif/tools/activate_idf_v6.0.1.sh -e)"
set +a
export PATH="$PATH:$SYSTEM_PATH"
cd "$ROOT_DIR"
"$IDF_PYTHON_ENV_PATH/bin/python" - "$PORT" "$DURATION" <<'PY' | tee "$LOG_DIR/$STAMP.log"
import sys
import time
import serial

port = sys.argv[1]
duration = float(sys.argv[2])

with serial.Serial(port, 115200, timeout=0.2) as ser:
    ser.dtr = False
    ser.rts = True
    time.sleep(0.1)
    ser.rts = False
    time.sleep(0.2)

    deadline = time.time() + duration
    while time.time() < deadline:
        data = ser.read(512)
        if data:
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()
PY

# Codex Operation Record

Timestamp: 2026-07-04 16:26:46 Asia/Shanghai

## User Goal

Fix the remaining touch inversion: `V` was accurate, but pressing visible `C` triggered `X`, and pressing visible `X` triggered `C`. Also clarify the meaning of the red/white bars in the status area and make the connection state clearer.

## Changes Made

- Updated `button_for_touch()` in `main/main.c`:
  - top raw touch third now maps to `X`
  - middle raw touch third maps to `V`
  - bottom raw touch third maps to `C`
- This compensates the current touch-controller vertical inversion while preserving the visible UI order: top `C`, middle `V`, bottom `X`.
- Replaced the previous temporary red/white status bars with a clearer status indicator:
  - connected: bright green block
  - disconnected: red X-style block

## Verification

- Built successfully with `./scripts/build.sh`.
- Flashed successfully to `/dev/cu.usbserial-10`.
- Captured serial startup log.
- Serial confirms:
  - `lcd init ok`
  - `BLE HID started; advertising as "FastKeyboard"`
  - `BLE HID connected`
  - `BLE GAP AUTH SUCCESS`

## Follow-up

The connection indicator is still deliberately simple. A later UI pass should replace the block graphics with proper readable text and a cleaner status area. Showing the connected Mac name may require host-side information that BLE HID does not expose directly to this firmware.

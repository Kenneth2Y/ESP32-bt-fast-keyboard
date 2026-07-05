# 2026-07-05 11:23:23 BMP Direct Render

## User Goal

Use the two BMP files added under `UIresources/` as the real TFT UI and render them directly on the MCU. The user recalled that the TFT path should use BGR.

## Files Read

- `UIresources/ast_keyboard_connected.bmp`
- `UIresources/ast_keyboard_disconnected.bmp`
- `main/main.c`
- `main/CMakeLists.txt`

## Changes Made

- Validated both BMP files as:
  - `320x240`
  - `24-bit`
  - uncompressed Windows BMP
  - top-down row order
- Generated MCU-ready bitmap arrays:
  - `main/ui_assets.c`
  - `main/ui_assets.h`
- Converted source BMP RGB pixels to BGR-lane RGB565 values for the current BGR test path.
- Replaced the LVGL UI rendering path with direct chunked `esp_lcd_panel_draw_bitmap()` rendering.
- Added `ui_assets.c` back into the main component build.
- Kept existing touch regions, BLE HID behavior, beeper feedback, and the currently correct display orientation.
- Kept the connected/disconnected state switch by drawing one of the two full-screen BMP arrays.

## Verification

- Built successfully with `./scripts/build.sh`.
- Flashed successfully to `/dev/cu.usbserial-10`.
- Captured serial logs after flashing.
- Confirmed logs show:
  - `lcd init ok`
  - `BLE HID started; advertising as "FastKeyboard"`
  - `BLE HID connected`
  - `ESP_HID_GAP: BLE GAP AUTH SUCCESS`

## Notes

- Visual color correctness still requires user confirmation on the physical TFT.
- This version is intentionally not committed yet because the BGR conversion result must be checked on the real panel first.

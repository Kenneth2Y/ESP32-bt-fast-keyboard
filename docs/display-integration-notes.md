# Display Integration Notes

This project uses a 2.8 inch 320x240 ST7789 TFT driven by ESP-IDF `esp_lcd`.

## Baseline 1.0 Display Path

- UI is rendered from two full-screen BMP assets:
  - `UIresources/ast_keyboard_connected.bmp`
  - `UIresources/ast_keyboard_disconnected.bmp`
- The BMP files are converted into MCU-ready arrays:
  - `main/ui_assets.c`
  - `main/ui_assets.h`
- Runtime rendering is direct:
  - `esp_lcd_panel_draw_bitmap()`
  - no LVGL in the final 1.0 UI path
- BLE connected/disconnected state switches between the two full-screen arrays.

## Important Color Findings

- The panel path that produced correct colors uses BGR-lane RGB565 conversion.
- Do not use `LV_COLOR_FORMAT_RGB565_SWAPPED`; it caused severe white horizontal stripe corruption on this panel.
- The display accepts 16-bit RGB565 pixel data, but the practical channel order must be treated as BGR for this board.
- Orange and bright colors were the most reliable visual indicators during calibration.
- Dark warm colors such as `(23, 18, 10)` could appear purple/noisy on the TFT. The generated assets clamp very dark pixels to neutral black.
- Near-white antialiased text pixels are thresholded to pure white to reduce colored edge noise on the low-DPI panel.

## ST7789 Initialization Notes

The vendor Arduino replacement file showed relevant ST7789 settings:

- `0x36` / `MADCTL`: memory access and RGB/BGR order.
- `0x3A = 0x55`: 16-bit RGB565 pixel format.
- `0xE0` and `0xE1`: gamma curves.
- No explicit `INVON` / `INVOFF` was present in the vendor file.

For this board, direction and touch alignment were verified with:

- `esp_lcd_panel_swap_xy(..., true)`
- `esp_lcd_panel_mirror(..., false, true)`

Changing these can break the already verified C/V/X touch mapping.

## UI Asset Rules

Use BMPs with:

- `320x240`
- `24-bit RGB`
- uncompressed Windows BMP
- top-down row order is supported
- no alpha
- no indexed color palette

Keep source BMPs in `UIresources/`. Regenerate `main/ui_assets.c` and `main/ui_assets.h` after changing them.

## Hardware and UX Notes

- The TFT DPI and panel quality are limited. Small antialiased fonts can look noisy even when data is correct.
- Thick simple shapes, high contrast text, and fewer thin borders look better.
- Baseline 1.0 intentionally removed outer frames and button outlines.
- The retained UI elements are:
  - top status area
  - C/V/X labels
  - three thick orange bars
- Touch and BLE HID logic are independent from the bitmap UI and should not be changed for visual-only revisions.

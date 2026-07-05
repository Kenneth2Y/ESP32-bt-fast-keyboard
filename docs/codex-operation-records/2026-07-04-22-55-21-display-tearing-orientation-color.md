# 2026-07-04 22:55:21 Display Tearing, Orientation, and Color Fix

## User Goal

Fix the LVGL screen version after the device still had these visible problems:

- Venetian-blind style tearing.
- Screen needed another 180 degree rotation.
- Colors still looked purple/blue instead of warm deep orange/black.

Bluetooth connection and touch input were reported as working.

## Files Read

- `main/main.c`
- `sdkconfig.defaults`
- ESP-IDF SPI LCD LVGL example:
  - `/Users/ky/.espressif/v6.0.1/esp-idf/examples/peripherals/lcd/spi_lcd_touch/main/spi_lcd_touch_example_main.c`
- LVGL refresh implementation:
  - `managed_components/lvgl__lvgl/src/core/lv_refr.c`

## Changes Made

- Added an SPI LCD transfer-done callback and moved `lv_display_flush_ready()` into that callback.
  - Reason: `esp_lcd_panel_draw_bitmap()` can queue SPI transfer work, so LVGL must not reuse the draw buffer until the LCD IO layer reports the transfer finished.
- Changed LVGL from one static draw buffer to two DMA-capable internal draw buffers.
  - Reason: avoid tearing/corruption caused by SPI DMA reading a buffer while LVGL starts drawing the next partial area.
- Reduced LCD pixel clock from `40MHz` to `20MHz`.
  - Reason: this static UI does not need high refresh speed, and the lower SPI rate reduces signal-integrity risk on the LCD module.
- Rotated the current LCD mapping 180 degrees by changing mirror flags from `(true, false)` to `(false, true)` while keeping `swap_xy=true`.
- Restored ST7789 color order from `RGB` to `BGR`.
- Switched LVGL display color format to `LV_COLOR_FORMAT_RGB565_SWAPPED`.
- Changed the Bluetooth status icon from blue to warm orange to avoid blue/purple dominance in the intended palette.

## Verification

- Built successfully with `./scripts/build.sh`.
- Flashed successfully to `/dev/cu.usbserial-10`.
- Captured serial logs after flashing.
- Confirmed logs show:
  - `lcd init ok`
  - `BLE HID connected`
  - `ESP_HID_GAP: BLE GAP AUTH SUCCESS`
- Visual orientation, tearing, and final color correctness still need user confirmation on the physical screen.

## Notes

- This change is intentionally not committed yet because the critical result is visual and cannot be verified from serial output alone.
- If colors are still wrong, the next likely test is to keep the transfer-done callback and DMA buffers but try `LV_COLOR_FORMAT_RGB565` versus `LV_COLOR_FORMAT_RGB565_SWAPPED`, or adjust the ST7789 `rgb_ele_order`.
- If direction is still wrong, the next likely test is another `esp_lcd_panel_mirror()` flag combination.

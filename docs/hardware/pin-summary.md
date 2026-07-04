# Hardware Pin Summary

Source files:

- `PIN定义.png`
- `esp32board.jpg`
- `实拍.png`

Board:

- Module: `ESP32-WROOM-32E`
- Product marking: `2.8" LCD Display ESP32-32E Resistance Touch`
- Display: 2.8 inch, 320x240, landscape target layout
- Touch: resistive touch panel, likely XPT2046/ADS7846-compatible SPI controller

Touch pins used by firmware:

| Signal | GPIO | Notes |
| --- | ---: | --- |
| `RTP_SCK` | 25 | Touch SPI clock |
| `RTP_DIN` | 32 | Touch controller data input |
| `RTP_CS` | 33 | Touch controller chip select |
| `RTP_IRQ` | 36 | Touch interrupt, input-only |
| `RTP_DOUT` | 39 | Touch controller data output, input-only |

TFT pins reserved for later UI work:

| Signal | GPIO |
| --- | ---: |
| `LCD_DC` | 2 |
| `LCD_CS` | 15 |
| `TFT_SDO` | 12 |
| `TFT_SDI` | 13 |
| `TFT_SCK` | 14 |
| `LCD_BL` | 21 |

The first firmware version logs touch coordinates over serial and maps the horizontal screen into three equal button zones: `C`, `V`, and `X`.

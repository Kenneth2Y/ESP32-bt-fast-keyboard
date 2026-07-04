# CY01 ESP32 Fast Keyboard

ESP32-WROOM-32E based Bluetooth HID shortcut keyboard for macOS.

Current firmware goal:

- Advertise as BLE HID keyboard: `CY01 Fast Keyboard`
- Use the resistive touch panel as three horizontal buttons
- Send macOS shortcuts:
  - `C`: `Command+C`
  - `V`: `Command+V`
  - `X`: `Command+X`
- Print boot, Bluetooth, touch, and key-send status through serial logs

The first implementation intentionally keeps the display UI minimal. Touch events are verified through serial logs first; the 320x240 black UI can be refined after the Bluetooth/touch loop is stable.

## Hardware Pins

Derived from the board images in this repository:

| Function | GPIO |
| --- | --- |
| Touch SCK (`RTP_SCK`) | 25 |
| Touch DIN (`RTP_DIN`) | 32 |
| Touch CS (`RTP_CS`) | 33 |
| Touch IRQ (`RTP_IRQ`) | 36 |
| Touch DOUT (`RTP_DOUT`) | 39 |
| TFT DC (`LCD_DC`) | 2 |
| TFT CS (`LCD_CS`) | 15 |
| TFT SCK | 14 |
| TFT SDI/MOSI | 13 |
| TFT SDO/MISO | 12 |
| TFT backlight (`LCD_BL`) | 21 |

## Build

```sh
./scripts/build.sh
```

## Flash

```sh
./scripts/flash.sh /dev/cu.usbserial-XXXX
```

## Monitor

```sh
./scripts/monitor.sh /dev/cu.usbserial-XXXX
```

On macOS, pair the device once in Bluetooth settings. After pairing, the firmware restarts advertising on disconnect, so reconnect should be automatic when the Mac and device are in range.

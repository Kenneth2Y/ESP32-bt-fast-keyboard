# Fast Keyboard firmware v1.0

这是 `v1.0` tag 对应的已验证固件归档。

- Tag: `v1.0`
- Source commit: `dbef9d32de3c2c30e7a982c7315eb623a71e6ac0`
- Build environment: ESP-IDF v6.0.1
- Target: ESP32 / ESP32-WROOM-32E
- App binary: `cy01_esp32_fast_keyboard.bin`

## Files

- `bootloader/bootloader.bin`
- `partition_table/partition-table.bin`
- `cy01_esp32_fast_keyboard.bin`
- `flash_args`
- `SHA256SUMS`

## Flash

From this directory:

```bash
python -m esptool --chip esp32 -b 460800 --before default-reset --after hard-reset write-flash "@flash_args"
```

Equivalent explicit command:

```bash
python -m esptool --chip esp32 -b 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-size 4MB --flash-freq 40m 0x1000 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0x10000 cy01_esp32_fast_keyboard.bin
```

## Notes

这个版本是当前硬件基线：BLE HID、触摸 C/V/X、GPIO23 蜂鸣反馈、BMP 静态 UI 已经在实物上验证。

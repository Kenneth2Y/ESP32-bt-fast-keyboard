/*
 * SPDX-FileCopyrightText: 2026 Kenneth2Y
 *
 * SPDX-License-Identifier: MIT
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_hid_gap.h"
#include "esp_hidd.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#include "ui_assets.h"

#define DEVICE_NAME "FastKeyboard"
#define MANUFACTURER_NAME "Kenneth2Y"

#define RTP_SCK_GPIO GPIO_NUM_25
#define RTP_DIN_GPIO GPIO_NUM_32
#define RTP_CS_GPIO GPIO_NUM_33
#define RTP_IRQ_GPIO GPIO_NUM_36
#define RTP_DOUT_GPIO GPIO_NUM_39

#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define LCD_HOST SPI2_HOST
#define LCD_MISO_GPIO GPIO_NUM_12
#define LCD_MOSI_GPIO GPIO_NUM_13
#define LCD_SCLK_GPIO GPIO_NUM_14
#define LCD_CS_GPIO GPIO_NUM_15
#define LCD_DC_GPIO GPIO_NUM_2
#define LCD_BL_GPIO GPIO_NUM_21
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define BEEPER_GPIO GPIO_NUM_23
#define BEEPER_PULSE_MS 80

#define TOUCH_X_MIN 350
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 350
#define TOUCH_Y_MAX 3800
#define TOUCH_DEBOUNCE_MS 260

#define HID_REPORT_ID_KEYBOARD 1
#define HID_MOD_LEFT_GUI 0x08
#define HID_KEY_C 0x06
#define HID_KEY_V 0x19
#define HID_KEY_X 0x1B

static const char *TAG = "CY01";

static esp_hidd_dev_t *s_hid_dev;
static esp_lcd_panel_handle_t s_lcd_panel;
static volatile bool s_hid_connected;

static const uint8_t s_keyboard_report_map[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, HID_REPORT_ID_KEYBOARD,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
    0x81, 0x03, 0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01,
    0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03, 0x91, 0x03,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
    0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xC0,
};

static esp_hid_raw_report_map_t s_report_maps[] = {
    {
        .data = s_keyboard_report_map,
        .len = sizeof(s_keyboard_report_map),
    },
};

static esp_hid_device_config_t s_hid_config = {
    .vendor_id = 0x16C0,
    .product_id = 0x05DF,
    .version = 0x0100,
    .device_name = DEVICE_NAME,
    .manufacturer_name = MANUFACTURER_NAME,
    .serial_number = "CY01-ESP32E-0001",
    .report_maps = s_report_maps,
    .report_maps_len = 1,
};

typedef struct {
    int x;
    int y;
    uint16_t raw_x;
    uint16_t raw_y;
} touch_point_t;

static void lcd_draw_ui(void)
{
    if (!s_lcd_panel) {
        return;
    }

    const uint16_t *image = s_hid_connected ? s_ui_bt_on : s_ui_bt_off;
    for (int row = 0; row < LCD_HEIGHT; row++) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_draw_bitmap(
            s_lcd_panel, 0, row, LCD_WIDTH, row + 1, image + row * LCD_WIDTH));
    }
}

static void lcd_init(void)
{
    ESP_LOGI(TAG, "lcd init: ST7789 MOSI=%d MISO=%d SCLK=%d CS=%d DC=%d BL=%d",
             LCD_MOSI_GPIO, LCD_MISO_GPIO, LCD_SCLK_GPIO, LCD_CS_GPIO, LCD_DC_GPIO, LCD_BL_GPIO);

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_BL_GPIO,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_BL_GPIO, 0);

    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SCLK_GPIO,
        .mosi_io_num = LCD_MOSI_GPIO,
        .miso_io_num = LCD_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * 40 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC_GPIO,
        .cs_gpio_num = LCD_CS_GPIO,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &s_lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_lcd_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_lcd_panel, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_lcd_panel, true));
    gpio_set_level(LCD_BL_GPIO, 1);

    lcd_draw_ui();
    ESP_LOGI(TAG, "lcd init ok");
}

static uint8_t spi_transfer_byte(uint8_t out)
{
    uint8_t in = 0;
    for (int bit = 7; bit >= 0; bit--) {
        gpio_set_level(RTP_DIN_GPIO, (out >> bit) & 0x01);
        esp_rom_delay_us(1);
        gpio_set_level(RTP_SCK_GPIO, 1);
        esp_rom_delay_us(1);
        in = (uint8_t)((in << 1) | gpio_get_level(RTP_DOUT_GPIO));
        gpio_set_level(RTP_SCK_GPIO, 0);
        esp_rom_delay_us(1);
    }
    return in;
}

static uint16_t touch_read_adc(uint8_t command)
{
    gpio_set_level(RTP_CS_GPIO, 0);
    spi_transfer_byte(command);
    uint16_t high = spi_transfer_byte(0x00);
    uint16_t low = spi_transfer_byte(0x00);
    gpio_set_level(RTP_CS_GPIO, 1);
    return (uint16_t)(((high << 8) | low) >> 3) & 0x0FFF;
}

static int map_adc_to_screen(uint16_t raw, uint16_t min, uint16_t max, int pixels)
{
    if (raw <= min) {
        return 0;
    }
    if (raw >= max) {
        return pixels - 1;
    }
    return (int)(((uint32_t)(raw - min) * (uint32_t)(pixels - 1)) / (uint32_t)(max - min));
}

static bool touch_read(touch_point_t *point)
{
    if (gpio_get_level(RTP_IRQ_GPIO) != 0) {
        return false;
    }

    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    const int samples = 5;
    for (int i = 0; i < samples; i++) {
        sum_x += touch_read_adc(0xD0);
        sum_y += touch_read_adc(0x90);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    point->raw_x = (uint16_t)(sum_x / samples);
    point->raw_y = (uint16_t)(sum_y / samples);
    point->x = map_adc_to_screen(point->raw_x, TOUCH_X_MIN, TOUCH_X_MAX, LCD_WIDTH);
    point->y = map_adc_to_screen(point->raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, LCD_HEIGHT);
    return true;
}

static char button_for_touch(const touch_point_t *point)
{
    if (point->y < LCD_HEIGHT / 3) {
        return 'X';
    }
    if (point->y < (LCD_HEIGHT * 2) / 3) {
        return 'V';
    }
    return 'C';
}

static uint8_t keycode_for_button(char button)
{
    switch (button) {
    case 'C':
        return HID_KEY_C;
    case 'V':
        return HID_KEY_V;
    case 'X':
        return HID_KEY_X;
    default:
        return 0;
    }
}

static void send_mac_shortcut(char button)
{
    uint8_t keycode = keycode_for_button(button);
    if (!keycode) {
        return;
    }
    if (!s_hid_connected || s_hid_dev == NULL) {
        ESP_LOGW(TAG, "touch %c ignored: BLE HID is not connected", button);
        return;
    }

    uint8_t report[8] = {0};
    report[0] = HID_MOD_LEFT_GUI;
    report[2] = keycode;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_hidd_dev_input_set(s_hid_dev, 0, HID_REPORT_ID_KEYBOARD, report, sizeof(report)));
    vTaskDelay(pdMS_TO_TICKS(45));

    memset(report, 0, sizeof(report));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_hidd_dev_input_set(s_hid_dev, 0, HID_REPORT_ID_KEYBOARD, report, sizeof(report)));
    ESP_LOGI(TAG, "sent Command+%c", button);
}

static void beep_feedback(void)
{
    gpio_set_level(BEEPER_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(BEEPER_PULSE_MS));
    gpio_set_level(BEEPER_GPIO, 0);
}

static void touch_task(void *arg)
{
    ESP_LOGI(TAG, "touch task running; layout: top=C middle=V bottom=X, screen %dx%d", LCD_WIDTH, LCD_HEIGHT);

    TickType_t last_action_tick = 0;
    while (true) {
        touch_point_t point = {0};
        if (!touch_read(&point)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - last_action_tick) < pdMS_TO_TICKS(TOUCH_DEBOUNCE_MS)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        last_action_tick = now;

        char button = button_for_touch(&point);
        ESP_LOGI(TAG, "touch raw=(%" PRIu16 ",%" PRIu16 ") mapped=(%d,%d) button=%c",
                 point.raw_x, point.raw_y, point.x, point.y, button);
        beep_feedback();
        send_mac_shortcut(button);

        while (gpio_get_level(RTP_IRQ_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

static void touch_init(void)
{
    gpio_config_t output_conf = {
        .pin_bit_mask = (1ULL << RTP_SCK_GPIO) | (1ULL << RTP_DIN_GPIO) | (1ULL << RTP_CS_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&output_conf));

    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << RTP_IRQ_GPIO) | (1ULL << RTP_DOUT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_conf));

    gpio_set_level(RTP_CS_GPIO, 1);
    gpio_set_level(RTP_SCK_GPIO, 0);
    gpio_set_level(RTP_DIN_GPIO, 0);

    ESP_LOGI(TAG, "touch pins: SCK=%d DIN=%d CS=%d IRQ=%d DOUT=%d",
             RTP_SCK_GPIO, RTP_DIN_GPIO, RTP_CS_GPIO, RTP_IRQ_GPIO, RTP_DOUT_GPIO);
}

static void beeper_init(void)
{
    gpio_config_t beeper_conf = {
        .pin_bit_mask = 1ULL << BEEPER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&beeper_conf));
    gpio_set_level(BEEPER_GPIO, 0);
    ESP_LOGI(TAG, "beeper pin: GPIO%d pulse=%dms", BEEPER_GPIO, BEEPER_PULSE_MS);
}

static void log_device_info(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    uint8_t mac[6] = {0};

    esp_chip_info(&chip_info);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_flash_get_size(NULL, &flash_size));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_read_mac(mac, ESP_MAC_BT));

    ESP_LOGI(TAG, "boot: %s", DEVICE_NAME);
    ESP_LOGI(TAG, "chip cores=%d revision=%d features=0x%08" PRIx32,
             chip_info.cores, chip_info.revision, chip_info.features);
    ESP_LOGI(TAG, "flash=%" PRIu32 "MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "bt_mac=%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDD_START_EVENT:
        ESP_LOGI(TAG, "BLE HID started; advertising as \"%s\"", DEVICE_NAME);
        esp_hid_ble_gap_adv_start();
        break;
    case ESP_HIDD_CONNECT_EVENT:
        s_hid_connected = true;
        ESP_LOGI(TAG, "BLE HID connected");
        lcd_draw_ui();
        break;
    case ESP_HIDD_DISCONNECT_EVENT:
        s_hid_connected = false;
        ESP_LOGI(TAG, "BLE HID disconnected: %s",
                 esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev),
                                               param->disconnect.reason));
        lcd_draw_ui();
        esp_hid_ble_gap_adv_start();
        break;
    case ESP_HIDD_PROTOCOL_MODE_EVENT:
        ESP_LOGI(TAG, "protocol mode map=%u mode=%s",
                 param->protocol_mode.map_index,
                 param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    case ESP_HIDD_OUTPUT_EVENT:
        ESP_LOGI(TAG, "output report id=%u len=%d", param->output.report_id, param->output.length);
        break;
    case ESP_HIDD_STOP_EVENT:
        ESP_LOGI(TAG, "BLE HID stopped");
        break;
    default:
        ESP_LOGD(TAG, "HID event=%" PRId32, id);
        break;
    }
}

void ble_hid_task_start_up(void)
{
    ESP_LOGD(TAG, "BLE GAP requested demo task start; CY01 touch task is already managed by app_main");
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    log_device_info();
    lcd_init();
    touch_init();
    beeper_init();

    ESP_ERROR_CHECK(esp_hid_gap_init(HIDD_BLE_MODE));
    ESP_ERROR_CHECK(esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_KEYBOARD, DEVICE_NAME));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler));
    ESP_ERROR_CHECK(esp_hidd_dev_init(&s_hid_config, ESP_HID_TRANSPORT_BLE, hidd_event_callback, &s_hid_dev));

    xTaskCreate(touch_task, "touch_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "main loop running");
}

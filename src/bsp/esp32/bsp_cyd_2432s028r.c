/*
 * BSP: CYD ESP32-2432S028R（2.8" 240x320 ILI9341 + XPT2046 电阻触摸）
 * 逻辑分辨率 320x240 横屏。
 */
#include "bsp.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/* ---------- 引脚定义（2432S028R 官方原理图） ---------- */
#define PIN_LCD_SCLK   14
#define PIN_LCD_MOSI   13
#define PIN_LCD_MISO   12
#define PIN_LCD_CS     15
#define PIN_LCD_DC     2
#define PIN_LCD_RST    4
#define PIN_LCD_BL     21
#define PIN_TOUCH_CS   33
#define PIN_TOUCH_IRQ  36

#define LCD_H_RES      320   /* 横屏逻辑分辨率 */
#define LCD_V_RES      240
#define LCD_SPI_HZ     (40 * 1000 * 1000)
#define DRAW_BUF_LINES 40

static const char *TAG = "bsp";

static SemaphoreHandle_t lvgl_mux;
static esp_lcd_panel_handle_t panel_handle;
static esp_lcd_touch_handle_t touch_handle;

void bsp_lvgl_lock(void)   { xSemaphoreTakeRecursive(lvgl_mux, portMAX_DELAY); }
void bsp_lvgl_unlock(void) { xSemaphoreGiveRecursive(lvgl_mux); }

/* ---------- LVGL 对接 ---------- */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

static uint32_t tick_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t x[1], y[1];
    uint8_t count = 0;
    esp_lcd_touch_read_data(touch_handle);
    if (esp_lcd_touch_get_coordinates(touch_handle, x, y, NULL, &count, 1) && count > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x[0];
        data->point.y = y[0];
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_task(void *arg)
{
    for (;;) {
        bsp_lvgl_lock();
        lv_timer_handler();
        bsp_lvgl_unlock();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

lv_display_t *bsp_get_display(void) { return lv_display_get_default(); }

void bsp_init(void)
{
    lvgl_mux = xSemaphoreCreateRecursiveMutex();

    /* 背光 GPIO */
    gpio_config_t bk = {
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk));

    /* SPI 总线（LCD 与触摸共用） */
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * DRAW_BUF_LINES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* LCD panel IO + ILI9341 */
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_SPI_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    /* 横屏 320x240 */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BL, 1));

    /* 触摸 XPT2046 */
    esp_lcd_panel_io_handle_t tp_io;
    esp_lcd_panel_io_spi_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_TOUCH_CS);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_cfg, &tp_io));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = PIN_TOUCH_IRQ,
        .levels = {.reset = 0, .interrupt = 0},
        .flags = {.swap_xy = true, .mirror_x = true, .mirror_y = false},
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &touch_handle));

    /* LVGL */
    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    size_t buf_sz = LCD_H_RES * DRAW_BUF_LINES * 2;
    void *buf1 = heap_caps_malloc(buf_sz, MALLOC_CAP_DMA);
    void *buf2 = heap_caps_malloc(buf_sz, MALLOC_CAP_DMA);
    ESP_ERROR_CHECK(buf1 && buf2 ? ESP_OK : ESP_ERR_NO_MEM);
    lv_display_set_buffers(disp, buf1, buf2, buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 12288, NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "BSP ready (2432S028R, %dx%d)", LCD_H_RES, LCD_V_RES);
}

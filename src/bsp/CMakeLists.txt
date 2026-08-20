# BSP 组件（ESP-IDF 构建时消费；desktop 等其他后端忽略本文件）
# 板型选择：后续由 Kconfig CONFIG_BOARD_* 决定，当前固定 CYD 2432S028R
idf_component_register(SRCS "esp32/bsp_cyd_2432s028r.c"
                            "esp32/bsp_wifi_esp32.c"
                       INCLUDE_DIRS "."
                       REQUIRES lvgl esp_lcd esp_lcd_ili9341 esp_lcd_touch_xpt2046
                                esp_timer driver esp_common esp_wifi esp_event esp_netif nvs_flash)

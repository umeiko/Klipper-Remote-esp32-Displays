file(GLOB UI_SRCS "*.c" "panels/*.c" "widgets/*.c" "assets/*.c")
idf_component_register(SRCS ${UI_SRCS}
                       INCLUDE_DIRS "." "panels" "widgets"
                       REQUIRES lvgl
                       PRIV_REQUIRES bsp)

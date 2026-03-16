idf_component_register(
	SRCS "esp_lcd_panel_rm67162.c"
	INCLUDE_DIRS "include"
	REQUIRES "esp_driver_gpio" "esp_driver_spi" "esp_lcd"
	PRIV_REQUIRES "driver" "esp_mm"
)

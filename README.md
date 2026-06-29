# esp\_lcd panel driver for rm67162 OLED display

## What is this and why?

This driver only works when the controller is connected over 3-Wire SPI,
and does not use DC pin ("16 bit transfer mode"), and tested only against:

* LilyGO T-Display-S3-AMOLED dev module.
* Waveshare ESP32-S3-AMOLED-1.91.

Why?

LilyGO provides a driver for this controller, which is

* Arduino-specific
* Uses polling SPI transactions (no dual buffer support possible)

Waveshare provides example code that uses driver for sh8601 controller, that
is apparently close enough to rm67162?...

## References

* T-Display-S3-AMOLED [https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)
* ESP-IDF [https://docs.espressif.com/projects/esp-idf/en/](https://docs.espressif.com/projects/esp-idf/en/)
* ESP-IDF `esp_lcd` component [https://github.com/espressif/esp-idf/tree/master/components/esp\_lcd](https://github.com/espressif/esp-idf/tree/master/components/esp_lcd)
* LVGL [https://docs.lvgl.io/](https://docs.lvgl.io/)
* Waveshare example [https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91/blob/main/02\_Example/ESP-IDF/03\_LVGL\_V8\_Test/LVGL\_Test\_90/main/example\_qspi\_with\_ram.c](https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91/blob/main/02_Example/ESP-IDF/03_LVGL_V8_Test/LVGL_Test_90/main/example_qspi_with_ram.c)
* sh8601 driver [https://github.com/espressif/esp-iot-solution/blob/master/components/display/lcd/esp\_lcd\_sh8601/esp\_lcd\_sh8601.c](https://github.com/espressif/esp-iot-solution/blob/master/components/display/lcd/esp_lcd_sh8601/esp_lcd_sh8601.c)

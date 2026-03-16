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

## References

* T-Display-S3-AMOLED [https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)
* ESP-IDF [https://docs.espressif.com/projects/esp-idf/en/](https://docs.espressif.com/projects/esp-idf/en/)
* ESP-IDF `esp_lcd` component [https://github.com/espressif/esp-idf/tree/master/components/esp\_lcd](https://github.com/espressif/esp-idf/tree/master/components/esp_lcd)
* LVGL [https://docs.lvgl.io/](https://docs.lvgl.io/)


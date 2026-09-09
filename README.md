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

Lilygo board has a "Display Power" GPIO pin that needs to be set to high
to enable display, and to low to turn it off. Waveshare board does not
have such pin; specify `-1` value in the board configuration strcture.

## Caveat(s)

Example application tries to shut everything down when the "BOOT" button
is pressed. It works, but there is a problem here:

Waveshare board consumes quite significant power after entering deep sleep.
It seems that excessive consumption is influenced by the operations that
turn off the display, notably by `LCD_CMD_DISPOFF` sent over SPI.
If anyone knows how to put the board to sleep properly, plese get in touch!
Lilygo board does not show such behaviour.

## References

* T-Display-S3-AMOLED [https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)
* ESP-IDF [https://docs.espressif.com/projects/esp-idf/en/](https://docs.espressif.com/projects/esp-idf/en/)
* ESP-IDF `esp_lcd` component [https://github.com/espressif/esp-idf/tree/master/components/esp\_lcd](https://github.com/espressif/esp-idf/tree/master/components/esp_lcd)
* LVGL [https://docs.lvgl.io/](https://docs.lvgl.io/)
* Waveshare example [https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91/blob/main/02\_Example/ESP-IDF/03\_LVGL\_V8\_Test/LVGL\_Test\_90/main/example\_qspi\_with\_ram.c](https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91/blob/main/02_Example/ESP-IDF/03_LVGL_V8_Test/LVGL_Test_90/main/example_qspi_with_ram.c)
* sh8601 driver [https://github.com/espressif/esp-iot-solution/tree/master/components/display/lcd/esp\_lcd\_sh8601](https://github.com/espressif/esp-iot-solution/tree/master/components/display/lcd/esp_lcd_sh8601)

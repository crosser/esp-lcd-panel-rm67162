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

## Appendix

### Lilygo init sequences:

```
	QSPI					SPI
					RM67162_CMD_WRCMDP	4
					RM67162_CMD_0X6A	0
					RM67162_CMD_WRCMDP	5
					RM67162_CMD_WRCMDP	7
					RM67162_CMD_0X07	0x4F
					RM67162_CMD_WRCMDP	1
					_LCD_CMD_CASET		2
					_LCD_CMD_RASET		0x73
					RM67162_CMD_WRCMDP	0x0A
					_LCD_CMD_DISPON		0x10
RM67162_CMD_WRCMDP	0		RM67162_CMD_WRCMDP	0
					LCD_CMD_WRDISBV		0xD0
					RM67162_CMD_WRCTRLD	0x20
					LCD_CMD_TEON		0
LCD_CMD_SLPOUT		- #120
RM67162_CMD_WRCMDP	5
RM67162_CMD_OVSS_CTL	5
RM67162_CMD_WRCMDP	1
RM67162_CMD_OVSS_VL	0x25
RM67162_CMD_WRCMDP	0
LCD_CMD_MADCTL 		x60
LCD_CMD_COLMOD		0x55		LCD_CMD_COLMOD		0x75
					RM67162_CMD_SETDSPI	0x80
					LCD_CMD_SLPOUT		0 #120
LCD_CMD_WRDISBV		0
LCD_CMD_DISPON		- #120		LCD_CMD_DISPON		0 #120
LCD_CMD_WRDISBV		0xD0
```

### Waveshare QSPI init sequence:

```
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {

    {0x11, (uint8_t []){0x00}, 0, 120},                 LCD_CMD_SLPOUT
    // {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},          LCD_CMD_STE
    // {0x35, (uint8_t []){0x00}, 1, 0},                LCD_CMD_TEON
    // {0x36, (uint8_t []){0xF0}, 1, 0},                LCD_CMD_MADCTL
    {0x3A, (uint8_t []){0x55}, 1, 0},  //16bits-RGB565  LCD_CMD_COLMOD
    {0x2A, (uint8_t []){0x00,0x00,0x02,0x17}, 4, 0},    LCD_CMD_CASET
    {0x2B, (uint8_t []){0x00,0x00,0x00,0xEF}, 4, 0},    LCD_CMD_RASET
    {0x51, (uint8_t []){0x00}, 1, 10},                  LCD_CMD_WRDISBV
    {0x29, (uint8_t []){0x00}, 0, 10},                  LCD_CMD_DISPON
    {0x51, (uint8_t []){0xFF}, 1, 0},                   LCD_CMD_WRDISBV
};
```

Another one:

```
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 120},                 LCD_CMD_SLPOUT
    {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},             LCD_CMD_STE
    {0x35, (uint8_t []){0x00}, 1, 0},                   LCD_CMD_TEON
    {0x53, (uint8_t []){0x20}, 1, 10},                  RM67162_CMD_WRCTRLD
    {0x51, (uint8_t []){0x00}, 1, 10},                  LCD_CMD_WRDISBV
    {0x29, (uint8_t []){0x00}, 0, 10},                  LCD_CMD_DISPON
    {0x51, (uint8_t []){0xFF}, 1, 0},                   LCD_CMD_WRDISBV
};
```

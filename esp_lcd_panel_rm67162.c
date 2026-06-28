/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2024-2026 Eugene Crosser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is an ESP-IDF component that adds support for rm67162 OLED chip
 * to the esp_lcd driver framework.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"

#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#define RM67162_CMD_DSTBON	0x4F	// Deep standby (RESX 0 > 3ms to wake)
#define RM67162_CMD_WRCTRLD	0x53	// Write display control . . B . D . . .
#define RM67162_CMD_RDCTRLD0	0x54	// Read disp contr (B-right, D-imming)
#define RM67162_CMD_RDCTRLD1	0x55	// RAD_ACL Control
#define RM67162_CMD_IMGEHCCTR0	0x58	// Set_color_enhance (three bits)
#define RM67162_CMD_IMGEHCCTR1	0x59	// Read_color_enhance
#define RM67162_CMD_CESLRCTR0	0x5A	// Set_color_enhance1
#define RM67162_CMD_CESLRCTR1	0x5B	// Read_color_enhance1

static const char *TAG = "lcd_panel.rm67162";

typedef struct {
	esp_lcd_panel_t base;
	esp_lcd_panel_io_handle_t io;
	int reset_gpio_num;
	bool reset_level;
	int x_gap;
	int y_gap;
	uint8_t fb_bits_per_pixel;
	uint8_t madctl_val;	// save current value of LCD_CMD_MADCTL register
	uint8_t colmod_val;	// save current value of LCD_CMD_COLMOD register
} rm67162_panel_t;

static esp_err_t panel_rm67162_del(esp_lcd_panel_t *panel)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);

	if (rm67162->reset_gpio_num >= 0) {
		gpio_reset_pin(rm67162->reset_gpio_num);
	}
	ESP_LOGD(TAG, "del rm67162 panel @%p", rm67162);
	free(rm67162);
	return ESP_OK;
}

/*
 * Without DC pin, this chip uses two bytes per byte of address,
 * and address itself is 16 bit wide. So here we need to construct a
 * 32 bit wide contraption containing high and low bytes of the address
 * interspersed with control bytes, and tell esp-idf's SPI driver that
 * this is our 32 bit long command. Luckily for us, subsequent data is
 * a simple sequence of bytes.
 * In the panel configuration, set .lcd_cmd_bits = 32, .lcd_param_bits = 8.
 *
 * Control byte, hi addr byte, control byte, lo addr byte, control byte, data
 *
 * 1 |R D H 0 0 0 0 0|A A A A A A A A|R D H 0 0 0 0 0|A A A A A A A A|
 * 0 |W C L 0 0 0 0 0|F E D C B A 9 8|W C L 0 0 0 0 0|7 6 5 4 3 2 1 0|
 *
 * Read or Write, data or command, hi or lo byte of 16bit address
 */
static esp_err_t rm67162_cmd_trans(esp_lcd_panel_io_handle_t io, int lcd_cmd,
		const void *param, size_t param_size)
{
	return esp_lcd_panel_io_tx_param(io, 0x02000000 | (lcd_cmd<<8),
			param, param_size);
}

static esp_err_t panel_rm67162_reset(esp_lcd_panel_t *panel)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);

	// perform hardware reset
	if (rm67162->reset_gpio_num >= 0) {
		int delays[2] = {300, 200};
		int lvl = rm67162->reset_level;

		ESP_RETURN_ON_ERROR(gpio_set_direction(rm67162->reset_gpio_num,
					GPIO_MODE_OUTPUT),
				TAG, "configure GPIO for RST line failed");
		for (int i = 0; i < 2; i++) {
			ESP_LOGD(TAG, "Set pin %d to %d",
				rm67162->reset_gpio_num, lvl);
			ESP_RETURN_ON_ERROR(gpio_set_level(
					rm67162->reset_gpio_num, lvl),
				TAG, "gpio_set_level active error");
			vTaskDelay(pdMS_TO_TICKS(delays[i]));
			lvl = !lvl;
		}
	} else {
		ESP_LOGD(TAG, "Performing software reset");
		ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
			(esp_lcd_panel_io_handle_t)rm67162->io,
			LCD_CMD_SWRESET, NULL, 0),
				TAG, "io tx param LCD_CMD_SWRESET failed");
		// spec, wait at least 5m before sending new command
		vTaskDelay(pdMS_TO_TICKS(20));
	}

	return ESP_OK;
}

static esp_err_t panel_rm67162_init(esp_lcd_panel_t *panel)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	esp_lcd_panel_io_handle_t io = rm67162->io;

	// LCD goes into sleep mode and display will be turned off
	// after power on reset, exit sleep mode first
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(io, LCD_CMD_SLPOUT, NULL, 0),
			TAG, "io tx param LCD_CMD_SLPOUT failed");
	vTaskDelay(pdMS_TO_TICKS(120));
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_MADCTL, (uint8_t[]) {rm67162->madctl_val,}, 1),
			TAG, "io tx param LCD_CMD_MADCTL failed");
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_COLMOD, (uint8_t[]) {rm67162->colmod_val,}, 1),
			TAG, "io tx param LCD_CMD_COLMOD failed");
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_WRDISBV, (uint8_t[]) {0,}, 1),
			TAG, "io tx param LCD_CMD_WRDISBV 0 failed");
	vTaskDelay(pdMS_TO_TICKS(120));
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_WRDISBV, (uint8_t[]) {0xD0,}, 1),
			TAG, "io tx param LCD_CMD_WRDISBV 0xD0 failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_draw_bitmap(esp_lcd_panel_t *panel, int x_start,
					   int y_start, int x_end, int y_end,
					   const void *color_data)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	esp_lcd_panel_io_handle_t io = rm67162->io;

	x_start += rm67162->x_gap;
	x_end += rm67162->x_gap;
	y_start += rm67162->y_gap;
	y_end += rm67162->y_gap;

	// define an area of frame memory where MCU can access
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_CASET, (uint8_t[]) {
			(x_start >> 8) & 0xFF, x_start & 0xFF,
			((x_end - 1) >> 8) & 0xFF, (x_end - 1) & 0xFF,
		}, 4), TAG, "io tx param LCD_CMD_CASET failed");
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		io, LCD_CMD_RASET, (uint8_t[]) {
			(y_start >> 8) & 0xFF, y_start & 0xFF,
			((y_end - 1) >> 8) & 0xFF, (y_end - 1) & 0xFF,
		}, 4), TAG, "io tx param LCD_CMD_RASET failed");
	// transfer frame buffer
	size_t len = (x_end - x_start) * (y_end - y_start)
			* rm67162->fb_bits_per_pixel / 8;
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(
		io, 0x32000000 | (LCD_CMD_RAMWR<<8), color_data, len),
			TAG, "io tx color LCD_CMD_RAMWR failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_invert_color(esp_lcd_panel_t *panel,
					    bool invert_color_data)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		(esp_lcd_panel_io_handle_t)rm67162->io,
		invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF,
		NULL, 0),
			TAG, "io tx param LCD_CMD_INVx failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_mirror(esp_lcd_panel_t *panel, bool mirror_x,
				      bool mirror_y)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	if (mirror_x) {
		rm67162->madctl_val |= LCD_CMD_MX_BIT;
	} else {
		rm67162->madctl_val &= ~LCD_CMD_MX_BIT;
	}
	if (mirror_y) {
		rm67162->madctl_val |= LCD_CMD_MY_BIT;
	} else {
		rm67162->madctl_val &= ~LCD_CMD_MY_BIT;
	}
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		(esp_lcd_panel_io_handle_t)rm67162->io,
		LCD_CMD_MADCTL,
		(uint8_t[]) {rm67162->madctl_val,}, 1),
			TAG, "io tx param LCD_CMD_MADCTL failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	if (swap_axes) {
		rm67162->madctl_val |= LCD_CMD_MV_BIT;
	} else {
		rm67162->madctl_val &= ~LCD_CMD_MV_BIT;
	}
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		(esp_lcd_panel_io_handle_t)rm67162->io,
		LCD_CMD_MADCTL,
		(uint8_t[]) {rm67162->madctl_val}, 1),
			TAG, "io tx param LCD_CMD_MADCTL failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_set_gap(esp_lcd_panel_t *panel, int x_gap,
				       int y_gap)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	rm67162->x_gap = x_gap;
	rm67162->y_gap = y_gap;
	return ESP_OK;
}

static esp_err_t panel_rm67162_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		(esp_lcd_panel_io_handle_t)rm67162->io,
		on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF, NULL, 0),
			TAG, "io tx param LCD_CMD_DISPx failed");
	return ESP_OK;
}

static esp_err_t panel_rm67162_sleep(esp_lcd_panel_t *panel, bool sleep)
{
	rm67162_panel_t *rm67162 = __containerof(panel, rm67162_panel_t, base);
	ESP_RETURN_ON_ERROR(rm67162_cmd_trans(
		(esp_lcd_panel_io_handle_t)rm67162->io,
		sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT, NULL, 0),
			TAG, "io tx param LCD_CMD_SLP%s failed",
			sleep ? "IN" : "OUT");
	vTaskDelay(pdMS_TO_TICKS(100));

	return ESP_OK;
}

static const esp_lcd_panel_t rm67162_base = {
	.reset = panel_rm67162_reset,
	.init = panel_rm67162_init,
	.del = panel_rm67162_del,
	.draw_bitmap = panel_rm67162_draw_bitmap,
	.invert_color = panel_rm67162_invert_color,
	.set_gap = panel_rm67162_set_gap,
	.mirror = panel_rm67162_mirror,
	.swap_xy = panel_rm67162_swap_xy,
	.disp_on_off = panel_rm67162_disp_on_off,
	.disp_sleep = panel_rm67162_sleep,
};

esp_err_t
esp_lcd_new_panel_rm67162(const esp_lcd_panel_io_handle_t io,
			  const esp_lcd_panel_dev_config_t *panel_dev_config,
			  esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
	esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
	esp_err_t ret = ESP_OK;
	rm67162_panel_t *rm67162 = NULL;
	ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel,
		ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
	rm67162 = calloc(1, sizeof(rm67162_panel_t));
	ESP_GOTO_ON_FALSE(rm67162,
		ESP_ERR_NO_MEM, err, TAG, "no mem for rm67162 panel");

	switch (panel_dev_config->rgb_ele_order) {
	/*
	 * Spec sheet says that LCD_CMD_MV_BIT is reversed for rm67162,
	 * but in reality it is not. I.e. in portrait orientation scanning
	 * goes left to right, top to bottom.
	 */
	case LCD_RGB_ELEMENT_ORDER_RGB:
		rm67162->madctl_val = 0;
		break;
	case LCD_RGB_ELEMENT_ORDER_BGR:
		rm67162->madctl_val = LCD_CMD_BGR_BIT;
		break;
	default:
		ESP_GOTO_ON_FALSE(false,
			ESP_ERR_NOT_SUPPORTED, err, TAG,
			"unsupported RGB element order");
		break;
	}

	uint8_t fb_bits_per_pixel = 0;
	switch (panel_dev_config->bits_per_pixel) {
	case 16:		// RGB565
		rm67162->colmod_val = 0x55;
		fb_bits_per_pixel = 16;
		break;
	case 18:		// RGB666
		rm67162->colmod_val = 0x66;
		// each color component (R/G/B) should occupy
		// the 6 high bits of a byte, which means 3 full bytes
		// are required for a pixel
		fb_bits_per_pixel = 24;
		break;
	case 24:		// RGB888
		rm67162->colmod_val = 0x77;
		fb_bits_per_pixel = 24;
		break;
	default:
		ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG,
				  "unsupported pixel width");
		break;
	}

	rm67162->fb_bits_per_pixel = fb_bits_per_pixel;
	rm67162->reset_gpio_num = panel_dev_config->reset_gpio_num;
	rm67162->reset_level = panel_dev_config->flags.reset_active_high;
	rm67162->io = io;
	rm67162->base = rm67162_base;

	*ret_panel = &(rm67162->base);
	ESP_LOGD(TAG, "new rm67162 panel @%p", rm67162);

	return ESP_OK;

 err:
	if (rm67162) {
		if (panel_dev_config->reset_gpio_num >= 0) {
			gpio_reset_pin(panel_dev_config->reset_gpio_num);
		}
		free(rm67162);
	}
	return ret;
}

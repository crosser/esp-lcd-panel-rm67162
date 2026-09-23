/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RM67162_CMD_OVSS_CTL	0x05	// OVSS control set elvss
#define RM67162_CMD_0X07	0x07	// ???
#define RM67162_CMD_DSTBON	0x4F	// Deep standby (RESX 0 > 3ms to wake)
#define RM67162_CMD_WRCTRLD	0x53	// Write display control . . B . D . . .
#define RM67162_CMD_RDCTRLD0	0x54	// Read disp contr (B-right, D-imming)
#define RM67162_CMD_RDCTRLD1	0x55	// RAD_ACL Control
#define RM67162_CMD_IMGEHCCTR0	0x58	// Set_color_enhance (three bits)
#define RM67162_CMD_IMGEHCCTR1	0x59	// Read_color_enhance
#define RM67162_CMD_CESLRCTR0	0x5A	// Set_color_enhance1
#define RM67162_CMD_CESLRCTR1	0x5B	// Read_color_enhance1
#define RM67162_CMD_0X6A	0x6A	// ???
#define RM67162_CMD_OVSS_VL	0x73	// Set OVSS voltage level
#define RM67162_CMD_SETDSI	0xC2	// Set DSI mode
#define RM67162_CMD_SETDSPI	0xC4	// Set DSPI mode
#define RM67162_CMD_WRCMDP	0xFE	// Write CMD mode page
#define RM67162_CMD_RDCMDP	0xFE	// Read CMD page status

typedef struct {
	uint8_t cmd;
	const uint8_t *data;  /* use "(uint8_t *)-1" as terminator */
	size_t data_bytes;  /* length of the data */
	unsigned int delay_ms;
} rm67162_init_cmd_t;

typedef struct {
	const rm67162_init_cmd_t *init_cmds;
	struct {
		unsigned int dc_less: 1; /*<! Set to 1 if use 32bit SPI cmds */
	} flags;
} rm67162_vendor_config_t;

/**
 * @brief Create LCD panel for model RM67162
 *
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_rm67162(const esp_lcd_panel_io_handle_t io,
				    const esp_lcd_panel_dev_config_t *
				    panel_dev_config,
				    esp_lcd_panel_handle_t * ret_panel);

#ifdef __cplusplus
}
#endif

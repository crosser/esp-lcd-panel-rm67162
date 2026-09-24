#ifndef _RM67162_CUSOTM_INIT
#define _RM67162_CUSOTM_INIT

static const rm67162_init_cmd_t rm67162_init_cmds[] = {
	{RM67162_CMD_WRCMDP, (uint8_t[]) {4}, 1, 0},	// SET APGE3
	{0x6A, (uint8_t[]) {0}, 1, 0},
	{RM67162_CMD_WRCMDP, (uint8_t[]) {5}, 1, 0},	// SET APGE4
	{RM67162_CMD_WRCMDP, (uint8_t[]) {7}, 1, 0},	// SET APGE6
	{0x07, (uint8_t[]) {0x4F}, 1, 0},
	{RM67162_CMD_WRCMDP, (uint8_t[]) {1}, 1, 0},	// SET APGE0
	{0x2A, (uint8_t[]) {2}, 1, 0},			// ~LCD_CMD_CASET
	{0x2B, (uint8_t[]) {0x73}, 1, 0},		// ~LCD_CMD_RASET
	{RM67162_CMD_WRCMDP, (uint8_t[]) {0x0A}, 1, 0},	// SET APGE9
	{0x29, (uint8_t[]) {0x10}, 1, 0},		// ~LCD_CMD_DISPON
	{RM67162_CMD_WRCMDP, (uint8_t[]) {0}, 1, 0},  // Reset command page
	// some stuff here in lilygo example
	{RM67162_CMD_WRCTRLD, (uint8_t[]) {0x20}, 1, 0},
	{RM67162_CMD_SETDSPI, (uint8_t[]) {0x80}, 1, 0},

	{0, (uint8_t *)-1, 0, 0},	/* Must terminate with pointer (-1) */
};

#endif

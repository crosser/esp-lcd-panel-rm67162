/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2024-2026 Eugene Crosser
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <lvgl.h>

void example_lvgl_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_label_set_text(label, "Hello Espressif, Hello LVGL. "
		      "This is an especially long text that is expected "
		      "to scroll circularly");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff),
				LV_PART_MAIN);
    lv_obj_set_width(label, lv_pct(80));
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000077),
			      LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

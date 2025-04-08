#include <stdbool.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_heap_caps.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include "esp_lcd_panel_rm67162.h"
#include "sdkconfig.h"
#include "lvgl.h"

#define TAG "lvgl_esplcd"

#if defined(CONFIG_HWE_DISPLAY_SPI1_HOST)
# define SPIx_HOST SPI1_HOST
#elif defined(CONFIG_HWE_DISPLAY_SPI2_HOST)
# define SPIx_HOST SPI2_HOST
#else
# error "SPI host 1 or 2 must be selected"
#endif

#if defined(CONFIG_HWE_DISPLAY_SPI_MODE0)
# define SPI_MODEx (0)
#elif defined(CONFIG_HWE_DISPLAY_SPI_MODE3)
# define SPI_MODEx (2)
#else
# error "SPI MODE0 or MODE3 must be selected"
#endif

#define SEND_BUF_SIZE ((CONFIG_HWE_DISPLAY_WIDTH * CONFIG_HWE_DISPLAY_HEIGHT \
                * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) / 10)

#define LV_TICK_PERIOD_MS 1

extern void example_lvgl_demo_ui(lv_display_t *disp);

static volatile bool is_running = true;

static void poweroff(void *arg)
{
	ESP_LOGI(TAG, "Power off timer called");
	is_running = false;
}

static bool IRAM_ATTR color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	lv_display_t *disp = (lv_display_t*)user_ctx;
	lv_display_flush_ready(disp);
	// Whether a high priority task has been waken up by this function
	return false; 
}

static void lv_tick_task(void *arg) {
	lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
		uint8_t *px_map)
{
	esp_lcd_panel_handle_t panel_handle =
		(esp_lcd_panel_handle_t)lv_display_get_user_data(disp_drv);
	ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle,
			area->x1, area->y1,
			area->x2 + 1, area->y2 + 1,
			(uint16_t *) px_map));
}

SemaphoreHandle_t xGuiSemaphore;

static void gui_task(void *pvParameter)
{
	xGuiSemaphore = xSemaphoreCreateMutex();
	ESP_LOGI(TAG, "Power up AMOLED");
	ESP_ERROR_CHECK(gpio_set_direction(CONFIG_HWE_DISPLAY_PWR,
				GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_HWE_DISPLAY_PWR,
				CONFIG_HWE_DISPLAY_PWR_ON_LEVEL));
	vTaskDelay(pdMS_TO_TICKS(500));

	ESP_LOGI(TAG, "Initialize SPI bus");
	ESP_ERROR_CHECK(spi_bus_initialize(SPIx_HOST,
		& (spi_bus_config_t) {
			.data0_io_num = CONFIG_HWE_DISPLAY_SPI_D0,
			.data1_io_num = CONFIG_HWE_DISPLAY_SPI_D1,
			.sclk_io_num = CONFIG_HWE_DISPLAY_SPI_SCK,
			.data2_io_num = CONFIG_HWE_DISPLAY_SPI_D2,
			.data3_io_num = CONFIG_HWE_DISPLAY_SPI_D3,
			.max_transfer_sz = SEND_BUF_SIZE + 8,
			.flags = SPICOMMON_BUSFLAG_MASTER
				| SPICOMMON_BUSFLAG_GPIO_PINS
				| SPICOMMON_BUSFLAG_QUAD,
		},
		SPI_DMA_CH_AUTO
	));
	ESP_LOGI(TAG, "Attach panel IO handle to SPI");
	esp_lcd_panel_io_handle_t io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
		(esp_lcd_spi_bus_handle_t)SPIx_HOST,
	       	& (esp_lcd_panel_io_spi_config_t) {
			.cs_gpio_num = CONFIG_HWE_DISPLAY_SPI_CS,
			.pclk_hz = CONFIG_HWE_DISPLAY_SPI_FREQUENCY,
			.lcd_cmd_bits = 32,
			.lcd_param_bits = 8,
#if defined(CONFIG_HWE_DISPLAY_SPI_SPI)
			.spi_mode = 0,
#elif defined(CONFIG_HWE_DISPLAY_SPI_QSPI)
			.spi_mode = 0,
			.flags.quad_mode = 1,
#elif defined(CONFIG_HWE_DISPLAY_SPI_OSPI)
			.spi_mode = 3,
			.flags.octal_mode = 1,
#else
# error "SPI single, quad and octal modes are supported"
#endif
			.trans_queue_depth = 17,
		},
	       	&io_handle
	));
	ESP_LOGI(TAG, "Attach vendor specific module");
	esp_lcd_panel_handle_t panel_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_rm67162(
		io_handle,
		& (esp_lcd_panel_dev_config_t) {
			.reset_gpio_num = CONFIG_HWE_DISPLAY_RST,
			.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
			.bits_per_pixel = 16,
		},
		&panel_handle
	));
	ESP_LOGI(TAG, "Reset panel");
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_LOGI(TAG, "Init panel");
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
	ESP_LOGI(TAG, "Turn on the screen");
	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
	// ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
	// Rotate 90 degrees clockwise:
	ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
	ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
	ESP_LOGI(TAG, "Turn on backlight");
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_HWE_DISPLAY_PWR,
				CONFIG_HWE_DISPLAY_PWR_ON_LEVEL));
	// panel_handle is ready, now deal with lvgl
	lv_init();
	// H and W exchanged because it lies on its side after rotation
	lv_display_t *disp = lv_display_create(CONFIG_HWE_DISPLAY_WIDTH,
			CONFIG_HWE_DISPLAY_HEIGHT);
	ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(
		io_handle,
		&(esp_lcd_panel_io_callbacks_t) {
			color_trans_done
		},
	       	disp));
	lv_display_set_user_data(disp, panel_handle);
	lv_display_set_flush_cb(disp, disp_flush);
	static lv_color_t *buf[2];
	for (int i = 0; i < 2; i++) {
		buf[i] = heap_caps_malloc(SEND_BUF_SIZE, MALLOC_CAP_DMA);
		assert(buf[i] != NULL);
	}
	lv_display_set_buffers(disp, buf[0], buf[1], SEND_BUF_SIZE,
			LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_90);

	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(
		&(esp_timer_create_args_t) {
			.callback = &lv_tick_task,
			.name = "periodic_gui",
		},
		&periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer,
		LV_TICK_PERIOD_MS * 1000));

	ESP_LOGI(TAG, "Display LVGL Scroll Text");
	example_lvgl_demo_ui(disp);
	while (is_running) {
		vTaskDelay(pdMS_TO_TICKS(10));
		if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
			lv_task_handler();
			xSemaphoreGive(xGuiSemaphore);
		}
		// ESP_LOGI(TAG, "is_running = %d", is_running);
	}
	ESP_LOGI(TAG, "Shutting down");
	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_HWE_DISPLAY_PWR,
				!CONFIG_HWE_DISPLAY_PWR_ON_LEVEL));
	esp_deep_sleep_start();
}

void app_main(void)
{
	static esp_timer_handle_t powerOffTimer;

	ESP_LOGI(TAG, "Launching gui task");
	/* Pinned to core 1. Core 0 will run bluetooth/wifi jobs. */
	xTaskCreatePinnedToCore(gui_task, "gui", 4096*2, NULL, 0, NULL, 1);
	ESP_ERROR_CHECK(esp_timer_create(&(esp_timer_create_args_t){
				.callback = poweroff,
				.arg = NULL,
				.name = "PowerOff",
			},
			&powerOffTimer));
	ESP_ERROR_CHECK(esp_timer_start_once(powerOffTimer, 15000000));
	ESP_LOGI(TAG, "Initialization complete");
}

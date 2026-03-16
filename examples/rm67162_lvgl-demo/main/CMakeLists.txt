set(srcs
	"lvgl_esplcd.c"
	"lvgl_demo_ui.c"
)

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS "."
	# EMBED_FILES image.jpg
)

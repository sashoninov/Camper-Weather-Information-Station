file(GLOB_RECURSE SRC_FILES "*.c")

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS 
        "."
        "squareline"
        "charts"
        "weather"
		"config"
    REQUIRES lvgl core network  audio
)

# 🔥 FIX за SquareLine
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=unused-value)
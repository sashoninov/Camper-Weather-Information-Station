#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sdcard_init(void);
esp_err_t sdcard_list_files(const char *path);

#ifdef __cplusplus
}
#endif

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void victron_init(void);
bool victron_read_line(char *out, int max_len);

#ifdef __cplusplus
}
#endif
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void http_gateway_init(void);

// ВРЪЩА ДЪЛЖИНА (>0 при успех, -1 при грешка)
int http_get(const char *url, char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

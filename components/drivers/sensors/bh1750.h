#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void bh1750_init(void);
float bh1750_read_lux(void);

#ifdef __cplusplus
}
#endif
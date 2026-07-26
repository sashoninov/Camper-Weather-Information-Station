#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "alpicool.h"

void ble_on_sync(void);
/* BLE low-level API, ползвано от alpicool.c */

int  alpicool_ble_connect(void);
void alpicool_ble_disconnect(void);
int alpicool_ble_query_status(void);

int  alpicool_ble_send_temp_short(int8_t t);
int  alpicool_ble_send_universal(int8_t temp,
                                 uint8_t mode,
                                 uint8_t power,
                                 uint8_t lock,
                                 uint8_t batt);

/* NimBLE sync callback – вика се от main.c */
void ble_on_sync(void);
bool alpicool_ble_is_connected(void);

#ifdef __cplusplus
}
#endif
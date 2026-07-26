#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int8_t  set_temp;
    int8_t  real_temp;
    uint8_t mode;          // 0 = MAX, 1 = ECO
    uint8_t power;         // 0 = OFF, 1 = ON
    uint8_t battery;       // %
    uint8_t batt_protect;  // 0..2
    uint8_t lock;          // 0/1
    float   voltage;

} alpicool_status_t;

const alpicool_status_t *alpicool_get_status_ptr(void);

/* HIGH-LEVEL API */
void alpicool_init(void);
void alpicool_get_status(alpicool_status_t *out);
void alpicool_request_status(void);
void alpicool_status_received(const alpicool_status_t *st);

void alpicool_set_temperature(int temp);

bool alpicool_ready_for_sleep(void);
void alpicool_prepare_sleep(void);

int  alpicool_set_temp(int8_t t);
int  alpicool_set_mode(uint8_t eco);
int  alpicool_set_power(uint8_t on);

#ifdef __cplusplus
}
#endif

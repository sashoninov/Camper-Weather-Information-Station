#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    I2C_OP_WRITE,
    I2C_OP_READ,
    I2C_OP_WRITE_READ
} i2c_op_t;

typedef struct {
    i2c_master_dev_handle_t dev;
    i2c_op_t op;

    uint8_t *tx_buf;
    size_t tx_len;

    uint8_t *rx_buf;
    size_t rx_len;

    TickType_t timeout;

    esp_err_t result;

    SemaphoreHandle_t done_sem;

} i2c_request_t;

void i2c_manager_init(void);
esp_err_t i2c_manager_submit(i2c_request_t *req);

#ifdef __cplusplus
}
#endif
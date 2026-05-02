#include "i2c_manager.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "I2C_MANAGER";

static QueueHandle_t i2c_queue;

#define I2C_QUEUE_LEN 20

static void i2c_manager_task(void *arg)
{
    i2c_request_t req;

    while (1) {
        if (xQueueReceive(i2c_queue, &req, portMAX_DELAY)) {

            switch (req.op) {

                case I2C_OP_WRITE:
                    req.result = i2c_master_transmit(
                        req.dev,
                        req.tx_buf,
                        req.tx_len,
                        req.timeout
                    );
                    break;

                case I2C_OP_READ:
                    req.result = i2c_master_receive(
                        req.dev,
                        req.rx_buf,
                        req.rx_len,
                        req.timeout
                    );
                    break;

                case I2C_OP_WRITE_READ:
                    // ❌ НЕ СЕ ПОЛЗВА
                    req.result = ESP_ERR_NOT_SUPPORTED;
                    break;
            }

            if (req.done_sem) {
                xSemaphoreGive(req.done_sem);
            }
        }
    }
}

void i2c_manager_init(void)
{
    i2c_queue = xQueueCreate(I2C_QUEUE_LEN, sizeof(i2c_request_t));

    xTaskCreatePinnedToCore(
        i2c_manager_task,
        "i2c_manager",
        4096,
        NULL,
        12,   // 🔥 по-нисък приоритет (LVGL safe)
        NULL,
        0
    );

    ESP_LOGI(TAG, "I2C manager started");
}

esp_err_t i2c_manager_submit(i2c_request_t *req)
{
    req->done_sem = xSemaphoreCreateBinary();

    if (xQueueSend(i2c_queue, req, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }

    if (xSemaphoreTake(req->done_sem, req->timeout) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(req->done_sem);

    return req->result;
}
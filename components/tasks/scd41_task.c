#include "scd41_task.h"
#include "app_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scd41.h"

void scd41_task(void *arg)
{
    if (scd41_init() != ESP_OK) {
        printf("❌ SCD41 init failed\n");
    } else {
        printf("✅ SCD41 init OK\n");
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // важно

    while (1)
    {
        uint16_t co2;
        float temp, hum;

        esp_err_t ret = scd41_read(&co2, &temp, &hum);

        if (ret == ESP_OK)
        {
            app_state.co2 = co2;
            app_state.scd_temp = temp;
            app_state.scd_humidity = hum;
            app_state.scd_valid = true;

           // printf("REAL: CO2: %d ppm | TEMP: %.2f C | HUM: %.2f %%\n",
            //       co2, temp, hum);
        }
        else
        {
            app_state.scd_valid = false;
            printf("❌ SCD41 read failed (%d)\n", ret);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // SCD41 цикъл
    }
}
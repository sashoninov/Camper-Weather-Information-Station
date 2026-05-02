#include "ds18b20_task.h"
#include "app_state.h"
#include "ds18b20_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_SENSORS 3

void ds18b20_task(void *arg)
{
    float temps[MAX_SENSORS];

    if (ds18b20_init() != ESP_OK) {
        printf("❌ DS18B20 init failed\n");
        vTaskDelete(NULL);
    }

    while (1)
    {
        if (ds18b20_read_all(temps, MAX_SENSORS) == ESP_OK)
        {
            float t_vun  = ds18b20_get_by_name("Temp Vun", temps);
            float frizer = ds18b20_get_by_name("Frizer", temps);
            float test   = ds18b20_get_by_name("Test", temps);

            // запис
            app_state.ds_temp[0] = t_vun;
            app_state.ds_temp[1] = frizer;
            app_state.ds_temp[2] = test;

            app_state.ds_valid[0] = (t_vun  > -100);
            app_state.ds_valid[1] = (frizer > -100);
            app_state.ds_valid[2] = (test   > -100);

            //printf("Vun: %.2f | Frizer: %.2f | Test: %.2f\n",
             //      t_vun, frizer, test);
        }
        else
        {
            for (int i = 0; i < 3; i++)
                app_state.ds_valid[i] = false;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
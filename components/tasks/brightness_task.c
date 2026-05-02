#include "bh1750.h"
#include "brightness.h"
#include "display.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
//#include "drivers/bh1750.h"
#include "lvgl.h"

static uint8_t current_brightness = 0;
static const char *TAG = "BRIGHTNESS";

void brightness_task(void *pv)
{
    float lux;
    uint8_t brightness;

    while(1) {

        lux = bh1750_read_lux();

        if(lux < 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        brightness = brightness_calculate(lux);

        // ESP_LOGI(TAG, "Lux: %.2f -> Brightness: %d", lux, brightness);

        int diff = brightness - current_brightness;

        if(diff > 0) {
            current_brightness += (diff > 3) ? 3 : 1;
        }
        else if(diff < 0) {
            current_brightness -= (diff < -3) ? 3 : 1;
        }

        display_set_brightness(current_brightness);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
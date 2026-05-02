#include "gpio_monitor.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"
#include "ui.h"
#include "esp_lvgl_port.h"

#include "app_state.h"
#include "audio_manager.h"
#include "audio_events.h"


#include <stdio.h>

// 👉 избери си GPIO
#define INPUT_GPIO GPIO_NUM_2

static int last_level = -1;

// ===================== UI UPDATE =====================

static void gpio_update_ui(int level)
{
    lvgl_port_lock(0);

    if (level == 0) {
        // LOW
        lv_label_set_text(ui_StatusWC, "Full");
        lv_img_set_src(ui_ImageWC, &ui_img_2072040035);
    } else {
        // HIGH
        lv_label_set_text(ui_StatusWC, "OK");
        lv_img_set_src(ui_ImageWC, &ui_img_1221855485);
    }

    lvgl_port_unlock();
}

// ===================== TASK =====================

void gpio_monitor_task(void *arg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INPUT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    while (1)
    {
        int level = gpio_get_level(INPUT_GPIO);

        if (level != last_level)
        {
            last_level = level;

            printf("GPIO level: %d\n", level);

            gpio_update_ui(level);

            // ===================== AUDIO WC FULL EVENT =====================
			if (level == 0) {   // FULL
				if (!app_state.wc_full_alerted) {
					audio_play_event(AUDIO_EVENT_TOILET_FULL);
					app_state.wc_full_alerted = true;
				}
			} else {            // OK
				app_state.wc_full_alerted = false;
			}

        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
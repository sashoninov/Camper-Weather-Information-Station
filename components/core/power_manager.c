#include "power_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "display/display.h"
#include "settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SLEEP_GPIO          GPIO_NUM_1
#define PA_GPIO             GPIO_NUM_53
#define DEBOUNCE_COUNT      5

static const char *TAG = "POWER";

bool power_sleep_active = false;
static bool sleeping = false;

static uint8_t get_target_brightness(void)
{
    uint8_t b = g_settings.manual_brightness;
    if (b < 1) b = 1;
    if (b > 100) b = 100;
    return b;
}

void power_manager_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << SLEEP_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);

    gpio_config_t pa = {
        .pin_bit_mask = 1ULL << PA_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pa);

    gpio_set_level(PA_GPIO, 1); // усилвател ON

    ESP_LOGI(TAG, "Power manager initialized");
}

static void enter_sleep_mode(void)
{
    if (sleeping) return;
    sleeping = true;
    power_sleep_active = true;

    ESP_LOGI(TAG, "Entering sleep mode");

    display_set_brightness(0);
    display_off();

    gpio_set_level(PA_GPIO, 0);

    esp_wifi_disconnect();
    esp_wifi_stop();
}

static void exit_sleep_mode(void)
{
    if (!sleeping) return;
    sleeping = false;
    power_sleep_active = false;

    ESP_LOGI(TAG, "Exiting sleep mode");

    esp_wifi_start();
    esp_wifi_connect();

    gpio_set_level(PA_GPIO, 1);

    display_on();
    display_set_brightness(get_target_brightness());
}

void power_manager_force_sleep(void)
{
    enter_sleep_mode();
}

void power_manager_force_wake(void)
{
    exit_sleep_mode();
}

void power_manager_task(void *arg)
{
    int stable_level = 1;
    int counter = 0;

    while (1)
    {
        int level = gpio_get_level(SLEEP_GPIO);

        if (level != stable_level) {
            counter++;
            if (counter >= DEBOUNCE_COUNT) {
                stable_level = level;
                counter = 0;

                if (stable_level == 1)
                    enter_sleep_mode();
                else
                    exit_sleep_mode();
            }
        } else {
            counter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

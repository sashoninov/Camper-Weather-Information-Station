#include "power_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"

// Display
#include "display/display.h"

#define SLEEP_GPIO GPIO_NUM_1

static const char *TAG = "POWER";
static bool sleeping = false;

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

    ESP_LOGI(TAG, "Power manager initialized");
}

static void enter_sleep_mode(void)
{
    if (sleeping) return;
    sleeping = true;

    ESP_LOGI(TAG, "Entering sleep mode");

    // Display OFF
    display_off();

    // WiFi disconnect + stop
    esp_wifi_disconnect();
    esp_wifi_stop();
}

static void exit_sleep_mode(void)
{
    if (!sleeping) return;
    sleeping = false;

    ESP_LOGI(TAG, "Exiting sleep mode");

    // WiFi ON
    esp_wifi_start();
    esp_wifi_connect();

    // Display ON
    display_on();
}

void power_manager_task(void *arg)
{
    while (1)
    {
        int level = gpio_get_level(SLEEP_GPIO);

        if (level == 0)
            enter_sleep_mode();
        else
            exit_sleep_mode();

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

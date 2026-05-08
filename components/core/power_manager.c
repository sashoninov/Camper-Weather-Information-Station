#include "power_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "display/display.h"
#include "settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SLEEP_GPIO          GPIO_NUM_1
#define DEBOUNCE_COUNT      5
#define FADE_STEP           5
#define FADE_DELAY_MS       10

static const char *TAG = "POWER";

static bool sleeping = false;
static uint8_t last_brightness = 100;   // последната яркост преди sleep

// Взимаме яркостта от Settings (слайдера)
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

    last_brightness = get_target_brightness();

    ESP_LOGI(TAG, "Power manager initialized");
}

static void enter_sleep_mode(void)
{
    if (sleeping) return;
    sleeping = true;

    ESP_LOGI(TAG, "Entering sleep mode");

    // 1) Запомняме яркостта от слайдера
    last_brightness = get_target_brightness();

    // 2) Fade-out
    for (int b = last_brightness; b >= 0; b -= FADE_STEP) {
        if (b < 0) b = 0;
        display_set_brightness((uint8_t)b);
        vTaskDelay(pdMS_TO_TICKS(FADE_DELAY_MS));
    }

    // 3) Гасим дисплея
    display_off();

    // 4) WiFi OFF
    esp_wifi_disconnect();
    esp_wifi_stop();
}

static void exit_sleep_mode(void)
{
    if (!sleeping) return;
    sleeping = false;

    ESP_LOGI(TAG, "Exiting sleep mode");

    // 1) WiFi ON
    esp_wifi_start();
    esp_wifi_connect();

    // 2) Пускаме панела
    display_on();

    // 3) Fade-in до стойността от слайдера
    uint8_t target = get_target_brightness();

    for (int b = 0; b <= target; b += FADE_STEP) {
        if (b > target) b = target;
        display_set_brightness((uint8_t)b);
        vTaskDelay(pdMS_TO_TICKS(FADE_DELAY_MS));
    }
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

                if (stable_level == 0)
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

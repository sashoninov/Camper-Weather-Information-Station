#include "ds18b20_driver.h"
#include "ds18b20.h"
#include "onewire_bus.h"
#include "onewire_bus_impl_rmt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

#define DS18B20_GPIO 20
#define MAX_SENSORS  3

static const char *TAG = "DS18B20";

static onewire_bus_handle_t bus = NULL;
static ds18b20_device_handle_t devs[MAX_SENSORS];
static int sensor_count = 0;

/* =========================
   ТВОИТЕ СЕНЗОРИ
   ========================= */

typedef struct {
    uint64_t address;
    const char *name;
    int index;
} ds_sensor_t;

static ds_sensor_t sensors[] = {
    {0xf32257d4450ed528ULL, "Temp Vun", -1},
    {0xe31bfdd44570b428ULL, "Frizer",   -1},
    {0x693ce104572ab328ULL, "Test",     -1},
};

#define SENSOR_COUNT (sizeof(sensors)/sizeof(sensors[0]))

/* ========================= */

esp_err_t ds18b20_init(void)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = DS18B20_GPIO,
    };

    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));

    onewire_device_iter_handle_t iter = NULL;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    onewire_device_t device;
    sensor_count = 0;

    ds18b20_config_t cfg = {};

    while (onewire_device_iter_get_next(iter, &device) == ESP_OK && sensor_count < MAX_SENSORS)
    {
        ESP_ERROR_CHECK(
            ds18b20_new_device_from_enumeration(&device, &cfg, &devs[sensor_count])
        );

        ESP_LOGI(TAG, "DS18B20 #%d found", sensor_count);
        sensor_count++;
    }

    onewire_del_device_iter(iter);

    ESP_LOGI(TAG, "Total sensors detected: %d", sensor_count);

    /* =========================
       MAPPING ПО АДРЕС (FIXED)
       ========================= */

    onewire_device_address_t addr;

    for (int i = 0; i < sensor_count; i++) {

        ds18b20_get_device_address(devs[i], &addr);

        ESP_LOGI(TAG, "Found ROM: 0x%016llX", addr);

        for (int s = 0; s < SENSOR_COUNT; s++) {
            if (sensors[s].address == addr) {
                sensors[s].index = i;

                ESP_LOGI(TAG, "Mapped %s → index %d",
                         sensors[s].name, i);
            }
        }
    }

    if (sensor_count == 0) {
        ESP_LOGE(TAG, "No DS18B20 found");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* ========================= */

esp_err_t ds18b20_read_all(float *temps, int max_count)
{
    if (sensor_count == 0) return ESP_FAIL;

    ESP_ERROR_CHECK(
        ds18b20_trigger_temperature_conversion_for_all(bus)
    );

    vTaskDelay(pdMS_TO_TICKS(750));

    for (int i = 0; i < sensor_count && i < max_count; i++) {
        ESP_ERROR_CHECK(
            ds18b20_get_temperature(devs[i], &temps[i])
        );
    }

    return ESP_OK;
}

/* ========================= */

float ds18b20_get_by_name(const char *name, float *temps)
{
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (strcmp(sensors[i].name, name) == 0) {
            if (sensors[i].index >= 0) {
                return temps[sensors[i].index];
            }
        }
    }

    return -1000.0f;
}
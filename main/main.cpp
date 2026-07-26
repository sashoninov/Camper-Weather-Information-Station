extern "C" {
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "sdcard.h"
#include "esp_hosted.h"
}

#include "power_manager.h"
#include "esp_wifi.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "alpicool.h"
#include "alpicool_ble.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_lvgl_port.h"
#include "wifi.h"
#include "time_sync.h"

// ⭐ Добавено
#include "sim_a7670.h"

#include "http_gateway.h"   // ⭐ НОВО


// TASKS
#include "time_task.h"
#include "weather_task.h"
#include "location_task.h"
#include "sensor_task.h"

#include "brightness_task.h"
#include "ds18b20_task.h"
#include "victron_task.h"
#include "gpio_monitor.h"

#include "brightness.h"

// Core
#include "app_state.h"

// Drivers
#include "i2c_bus.h"
#include "i2c_bus1.h"
#include "display.h"

#include "ds3231.h"

// UI
#include "ui.h"
#include "settings_ui.h"
#include "alpicool_ui.h"
#include "global_touch.h"

// Logic
#include "settings.h"

// Sensors

#include "bh1750.h"
#include "ina3221.h"
#include "mpu6050.h"
#include "ds18b20.h"
#include "ds18b20_driver.h"

#include "calibration.h"

// GPS
#include "gps.h"

// NEW
#include "ui_Nivel.h"

// Audio
#include "audio_manager.h"
#include "audio_events.h"

static const char *TAG = "MAIN";

extern "C" void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// =========================
// TOUCH -> WAKE
// =========================
void global_touch_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED ||
        code == LV_EVENT_CLICKED ||
        code == LV_EVENT_PRESSING)
    {
        brightness_touch_event();
    }
}

// =========================
// UI NIVEL TASK
// =========================
static void ui_nivel_task(void *arg)
{
    while (1) {
        ui_nivel_update();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// =========================
// MAIN
// =========================
extern "C" void app_main(void)
{
    ESP_LOGI("MAIN", "System boot");

    ////////////////////////////////
    /// SD CARD ///
    if (sdcard_init() == ESP_OK) {
        sdcard_list_files("/sdcard/audio/system");
    }

    ESP_LOGI(TAG, "System boot");

    // Silence WiFi6 RPC spam
    esp_log_level_set("rpc_req",  ESP_LOG_ERROR);
    esp_log_level_set("rpc_rsp",  ESP_LOG_ERROR);
    esp_log_level_set("RPC_WRAP", ESP_LOG_ERROR);
    esp_log_level_set("wifi",     ESP_LOG_ERROR);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    app_state_init();

    // LOAD SETTINGS
    if (!settings_load(&g_settings)) {
        ESP_LOGW(TAG, "Using default settings");
    }

    // ⭐ Инициализация на HTTP gateway (mutex за всички заявки)
    http_gateway_init();

    // ⭐ Зареждаме /GSM 
    ESP_LOGI(TAG, "Starting GSM modem...");

    sim_a7670_init();
    sim_a7670_start();
    sim_a7670_monitor_start();

	ESP_LOGI(TAG, "Starting GPS...");
	
	if (!gps_init())
	{
		ESP_LOGE(TAG, "gps_init() failed");
	}
	
	if (!gps_start())
	{
		ESP_LOGE(TAG, "gps_start() failed");
	}

    // I2C
    ESP_ERROR_CHECK(i2c_bus_init());
    ESP_ERROR_CHECK(i2c1_bus_init());

    i2c1_scan();

    // RTC
    ds3231_init();

    struct tm rtc_time = {0};
    if (ds3231_get_time(&rtc_time)) {
        struct timeval tv = {
            .tv_sec  = mktime(&rtc_time),
            .tv_usec = 0
        };
        settimeofday(&tv, NULL);
        ESP_LOGI("RTC", "Loaded time from DS3231");
    }

    // WIFI SoftAP
    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(wifi_start());


    // BLE 
    esp_hosted_connect_to_slave();
    esp_hosted_bt_controller_init();
    esp_hosted_bt_controller_enable();

    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_on_sync;
    nimble_port_freertos_init(host_task);    

    time_sync_init();

    // DISPLAY
    ESP_ERROR_CHECK(display_init());

    // UI
    ui_init();

    lvgl_port_lock(0);
    ui_update_settings_screen();
    ui_settings_init_events();
    lvgl_port_unlock();

    lv_obj_add_event_cb(lv_scr_act(), global_touch_handler, LV_EVENT_ALL, NULL);

    // 🔹 Alpicool UI events
    lv_obj_add_event_cb(ui_ImgButton3, ui_event_power, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_ImgButton4, ui_event_mode,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_SliderTemp, ui_event_temp,  LV_EVENT_VALUE_CHANGED, NULL);

    // AUDIO
    audio_manager_init();
    audio_play_event(AUDIO_EVENT_BOOT);

    // UI NIVEL TASK
    xTaskCreate(ui_nivel_task, "ui_nivel", 4096, NULL, 5, NULL);

    // SENSORS

    bh1750_init();
    ina3221_init();
    mpu6050_init();

    if (calibration_load())
        ESP_LOGI("CALIB", "Calibration loaded");

    ESP_LOGI(TAG, "Calibrating MPU6050...");
    mpu6050_calibrate();


    // Alpicool
    alpicool_init();
 
    alpicool_status_t st = {0};
    alpicool_get_status(&st);
    alpicool_ui_update(&st);   

    // POWER MANAGER
    power_manager_init();
    xTaskCreate(power_manager_task, "power_manager", 4096, NULL, 5, NULL);

    // TASKS
    xTaskCreate(time_task,         "time",         4096, NULL, 5, NULL);
    xTaskCreate(time_sync_task,    "time_sync",    4096, NULL, 5, NULL);
    xTaskCreate(location_task,     "location",     16384, NULL, 5, NULL);

    while (!time_sync_is_valid()) {
        ESP_LOGI("MAIN", "Waiting for valid time...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    xTaskCreate(weather_task,      "weather",      16384, NULL, 5, NULL);
    xTaskCreate(sensor_task,       "sensor_task",  4096, NULL, 5, NULL);

    xTaskCreate(ds18b20_task,      "ds18b20",      4096, NULL, 5, NULL);
    xTaskCreate(victron_task,      "victron",      4096, NULL, 5, NULL);
    xTaskCreate(brightness_task,   "brightness",   4096, NULL, 5, NULL);
    xTaskCreate(gpio_monitor_task, "gpio_monitor", 4096, NULL, 5, NULL);

    

    ESP_LOGI(TAG, "System ready");

    vTaskDelete(NULL);
}

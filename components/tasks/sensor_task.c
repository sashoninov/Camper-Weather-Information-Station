#include "ina3221.h"
#include "tank_levels.h"
#include "ui_update_ina.h"
#include "ui_update_mpu.h"
#include "ui_update_scd41.h"
#include "ui_update_ds18b20.h"
#include "ui_update_victron.h"
#include "esp_lvgl_port.h"
#include <math.h>

#include "mpu6050.h"
#include "scd41.h"
#include "app_state.h"
#include "calibration.h"
#include "status.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void sensor_task(void *arg)
{
    if (ina3221_init() != ESP_OK) {
        printf("❌ INA3221 init failed\n");
    } else {
        printf("✅ INA3221 init OK\n");
    }

    if (mpu6050_init() != ESP_OK) {
        printf("❌ MPU6050 init failed\n");
    } else {
        printf("✅ MPU6050 init OK\n");
    }

    // =========================
    // LOAD CALIBRATION FROM NVS
    // =========================
    if (calibration_load()) {
        calibration_data_t c;
        calibration_get(&c);
        printf("Loaded calibration: pitch=%.2f roll=%.2f\n", c.pitch_offset, c.roll_offset);
    } else {
        printf("No calibration found in NVS\n");
    }

    ina3221_data_t ina;
    mpu6050_data_t mpu;

    uint32_t last_ina = 0;
    uint32_t last_scd = 0;

    static uint32_t calib_done_time = 0;
    static bool motion_detected = false;
    static uint32_t motion_stop_time = 0;

    static float last_grey = 0;
    static float last_clean = 0;

    while (1)
    {
        uint32_t now = xTaskGetTickCount();

        // =========================
        // MPU CALIBRATION (BUTTON)
        // =========================
        if (app_state.request_calibration)
        {
            app_state.request_calibration = false;

            vTaskDelay(pdMS_TO_TICKS(500));
            mpu6050_calibrate();

            calibration_data_t c;
            calibration_get(&c);
            calibration_save();

            printf("✅ MPU calibrated and saved: pitch=%.2f roll=%.2f\n",
                   c.pitch_offset, c.roll_offset);

            app_state.calibration_done = true;
            calib_done_time = xTaskGetTickCount();
        }

        // =========================
        // INA3221
        // =========================
        if (now - last_ina > pdMS_TO_TICKS(20000))
        {
            if (ina3221_read(&ina) == ESP_OK)
            {
                app_state.ina_voltage[0] = ina.voltage[0];
                app_state.ina_voltage[1] = ina.voltage[1];
                app_state.ina_voltage[2] = ina.voltage[2];

                float ch1 = ina.voltage[0];
                g_status.starter_v = ch1;

                float ch2 = ina.voltage[1];
                float ch3 = ina.voltage[2];

                float battery = ch1;

                bool allow_water_update = true;

                if (motion_detected)
                    allow_water_update = false;
                else if (xTaskGetTickCount() - motion_stop_time < pdMS_TO_TICKS(3000))
                    allow_water_update = false;

                float grey;
                float clean;

                if (allow_water_update)
                {
                    grey = convert_grey_water_percent(ch2);
                    clean = convert_clean_water_liters(ch3);

                    last_grey = grey;
                    last_clean = clean;
                }
                else
                {
                    grey = last_grey;
                    clean = last_clean;
                }

                lvgl_port_lock(0);
                ui_update_ina(battery, grey, clean);
                lvgl_port_unlock();
            }

            last_ina = now;
        }

        // =========================
        // MPU6050
        // =========================
        if (mpu6050_read(&mpu) == ESP_OK)
        {
            app_state.mpu.ax = mpu.ax;
            app_state.mpu.ay = mpu.ay;
            app_state.mpu.az = mpu.az;

            app_state.mpu.gx = mpu.gx;
            app_state.mpu.gy = mpu.gy;
            app_state.mpu.gz = mpu.gz;

            float accel = sqrtf(mpu.ax * mpu.ax +
                                mpu.ay * mpu.ay +
                                mpu.az * mpu.az);

            if (accel > 1.1f || accel < 0.9f)
            {
                motion_detected = true;
            }
            else
            {
                if (motion_detected)
                {
                    motion_detected = false;
                    motion_stop_time = xTaskGetTickCount();
                }
            }

            mpu_angles_t a = mpu6050_get_angles_from_data(&mpu);

            static float pitch_f = 0;
            static float roll_f = 0;

            pitch_f = pitch_f * 0.9f + a.pitch * 0.1f;
            roll_f  = roll_f * 0.9f + a.roll * 0.1f;

            g_status.pitch = pitch_f;
            g_status.roll  = roll_f;


            app_state.mpu.pitch = pitch_f;
            app_state.mpu.roll  = roll_f;

            lvgl_port_lock(0);
            ui_update_mpu(pitch_f, roll_f);
            lvgl_port_unlock();
        }

        if (app_state.calibration_done &&
            xTaskGetTickCount() - calib_done_time > pdMS_TO_TICKS(2000))
        {
            app_state.calibration_done = false;
        }

        // =========================
        // UI UPDATE SCD41
        // =========================
        static uint32_t last_ui = 0;
        
        if (now - last_ui > pdMS_TO_TICKS(1000))
        {
            lvgl_port_lock(0);
            ui_update_scd41();
            lvgl_port_unlock();
        
            last_ui = now;
        }

        // =========================
        // UI UPDATE DS18B20
        // =========================
        static uint32_t last_ds_ui = 0;

        if (now - last_ds_ui > pdMS_TO_TICKS(1000))
        {
            lvgl_port_lock(0);
            ui_update_ds18b20();
            lvgl_port_unlock();

            last_ds_ui = now;
        }

        // =========================
        // UI UPDATE VICTRON
        // =========================        
        static uint32_t last_victron_ui = 0;
        
        if (now - last_victron_ui > pdMS_TO_TICKS(1000))
        {
            lvgl_port_lock(0);
            ui_update_victron();
            lvgl_port_unlock();
        
            last_victron_ui = now;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

#include "bme680.h"

#include <string.h>

#include "esp_log.h"
#include "esp_rom_sys.h"

#include "i2c_bus1.h"

#define TAG "BME680"

#define BME680_AMBIENT_TEMPERATURE   25

static struct bme68x_dev g_dev;
static i2c_master_dev_handle_t g_i2c = NULL;
static bool g_initialized = false;

/* -------------------------------------------------------------------------- */
/* Bosch I2C callbacks                                                        */
/* -------------------------------------------------------------------------- */

static BME68X_INTF_RET_TYPE bme680_i2c_read(
    uint8_t reg_addr,
    uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    i2c_master_dev_handle_t dev =
        (i2c_master_dev_handle_t)intf_ptr;

    if (dev == NULL)
    {
        return BME68X_E_NULL_PTR;
    }

    esp_err_t err =
        i2c1_bus_write_read(
            dev,
            &reg_addr,
            1,
            reg_data,
            len);

    return (err == ESP_OK) ?
            BME68X_INTF_RET_SUCCESS :
            BME68X_E_COM_FAIL;
}

static BME68X_INTF_RET_TYPE bme680_i2c_write(
    uint8_t reg_addr,
    const uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    i2c_master_dev_handle_t dev =
        (i2c_master_dev_handle_t)intf_ptr;

    if (dev == NULL)
    {
        return BME68X_E_NULL_PTR;
    }

    if (len > 31)
    {
        return BME68X_E_INVALID_LENGTH;
    }

    uint8_t tx[32];

    tx[0] = reg_addr;

    memcpy(&tx[1], reg_data, len);

    esp_err_t err =
        i2c1_bus_write(
            dev,
            tx,
            len + 1);

    return (err == ESP_OK) ?
            BME68X_INTF_RET_SUCCESS :
            BME68X_E_COM_FAIL;
}

static void bme680_delay_us(
    uint32_t period,
    void *intf_ptr)
{
    (void)intf_ptr;

    esp_rom_delay_us(period);
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

bool bme680_is_initialized(void)
{
    return g_initialized;
}

void bme680_deinit(void)
{
    g_initialized = false;
    g_i2c = NULL;

    memset(&g_dev, 0, sizeof(g_dev));
}

bool bme680_init(void)
{
    if (g_initialized)
    {
        return true;
    }

    esp_err_t err =
        i2c1_bus_add_device(
            BME68X_I2C_ADDR_LOW,
            &g_i2c);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Failed to add I2C device (%s)",
                 esp_err_to_name(err));

        return false;
    }

    memset(&g_dev, 0, sizeof(g_dev));

    g_dev.intf = BME68X_I2C_INTF;
    g_dev.intf_ptr = g_i2c;
    g_dev.read = bme680_i2c_read;
    g_dev.write = bme680_i2c_write;
    g_dev.delay_us = bme680_delay_us;
    g_dev.amb_temp = BME680_AMBIENT_TEMPERATURE;

    int8_t rslt = bme68x_init(&g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_init() failed (%d)",
                 rslt);

        return false;
    }

    struct bme68x_conf conf = {
        .os_hum  = BME68X_OS_2X,
        .os_temp = BME68X_OS_4X,
        .os_pres = BME68X_OS_8X,
        .filter  = BME68X_FILTER_SIZE_3,
        .odr     = BME68X_ODR_NONE
    };

    rslt = bme68x_set_conf(&conf, &g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_set_conf() failed (%d)",
                 rslt);

        return false;
    }

    struct bme68x_heatr_conf heatr = {
        .enable = BME68X_ENABLE,
        .heatr_temp = 320,
        .heatr_dur = 150,
        .heatr_temp_prof = NULL,
        .heatr_dur_prof = NULL,
        .profile_len = 0,
        .shared_heatr_dur = 0
    };

    rslt =
        bme68x_set_heatr_conf(
            BME68X_FORCED_MODE,
            &heatr,
            &g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_set_heatr_conf() failed (%d)",
                 rslt);

        return false;
    }

    g_initialized = true;

    ESP_LOGI(TAG,
             "BME680 initialized");

    return true;
}

/* -------------------------------------------------------------------------- */
/* Read one measurement                                                       */
/* -------------------------------------------------------------------------- */

bool bme680_read(sensor_data_t *data)
{
    if (!g_initialized)
    {
        return false;
    }

    if (data == NULL)
    {
        return false;
    }

    struct bme68x_conf conf;

    int8_t rslt = bme68x_get_conf(&conf, &g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_get_conf() failed (%d)",
                 rslt);

        return false;
    }

    rslt = bme68x_set_op_mode(
                BME68X_FORCED_MODE,
                &g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_set_op_mode() failed (%d)",
                 rslt);

        return false;
    }

    uint32_t meas_time =
        bme68x_get_meas_dur(
            BME68X_FORCED_MODE,
            &conf,
            &g_dev);

    /* Heater time + small safety margin */
    meas_time += (150U * 1000U);
    meas_time += 5000U;

    g_dev.delay_us(
        meas_time,
        g_dev.intf_ptr);

    struct bme68x_data sensor;
    uint8_t n_fields = 0;

    memset(&sensor, 0, sizeof(sensor));

    rslt =
        bme68x_get_data(
            BME68X_FORCED_MODE,
            &sensor,
            &n_fields,
            &g_dev);

    if (rslt != BME68X_OK)
    {
        ESP_LOGE(TAG,
                 "bme68x_get_data() failed (%d)",
                 rslt);

        return false;
    }

    if (n_fields == 0)
    {
        ESP_LOGW(TAG,
                 "No measurement available");

        return false;
    }

    if ((sensor.status & BME68X_NEW_DATA_MSK) == 0)
    {
        ESP_LOGW(TAG,
                 "No new data");

        return false;
    }

    memset(data, 0, sizeof(sensor_data_t));

    data->temperature = sensor.temperature;
    data->humidity = sensor.humidity;

    /* Bosch API returns pressure in Pascal */
    data->pressure = sensor.pressure / 100.0f;

    data->gas_resistance = sensor.gas_resistance;

    /* Filled later by environment.c */
    data->iaq = 0;
    data->air_quality_level = 0;

    data->valid = true;

    return true;
}
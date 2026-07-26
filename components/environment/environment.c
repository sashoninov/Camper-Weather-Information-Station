#include "environment.h"

#include <string.h>

#include "bme680.h"

static sensor_data_t s_environment;

static float s_gas_baseline = 0.0f;
static bool s_baseline_ready = false;

#define HUM_BASELINE (40.0f)
#define GAS_WEIGHT (0.75f)
#define HUM_WEIGHT (0.25f)

/* -------------------------------------------------------------------------- */
/* Local                                                                      */
/* -------------------------------------------------------------------------- */

static void environment_calculate_iaq(sensor_data_t *data)
{
    if (data == NULL || !data->valid)
        return;

    if (!s_baseline_ready)
    {
        s_gas_baseline = data->gas_resistance;
        s_baseline_ready = true;
    }

    if (data->gas_resistance > s_gas_baseline)
    {
        s_gas_baseline = s_gas_baseline * 0.99f + data->gas_resistance * 0.01f;
    }

    float hum_offset = data->humidity - HUM_BASELINE;
    float hum_score = (data->humidity <= HUM_BASELINE)
        ? (HUM_BASELINE + hum_offset) / HUM_BASELINE * 100.0f
        : (HUM_BASELINE - hum_offset) / HUM_BASELINE * 100.0f;

    if (hum_score < 0.0f) hum_score = 0.0f;
    if (hum_score > 100.0f) hum_score = 100.0f;

    float gas_score = (s_gas_baseline / data->gas_resistance) * 100.0f;
    if (gas_score > 100.0f) gas_score = 100.0f;
    if (gas_score < 0.0f) gas_score = 0.0f;

    float iaq = gas_score * GAS_WEIGHT + hum_score * HUM_WEIGHT;

    data->iaq = (uint16_t)iaq;

    if (iaq >= 90.0f) data->air_quality_level = 0;
    else if (iaq >= 75.0f) data->air_quality_level = 1;
    else if (iaq >= 60.0f) data->air_quality_level = 2;
    else if (iaq >= 40.0f) data->air_quality_level = 3;
    else data->air_quality_level = 4;
}

/* -------------------------------------------------------------------------- */
/* Public                                                                     */
/* -------------------------------------------------------------------------- */

bool environment_init(void)
{
    memset(&s_environment, 0, sizeof(s_environment));

    return bme680_init();
}

bool environment_update(void)
{
    sensor_data_t data;

    if (!bme680_read(&data))
    {
        return false;
    }

    environment_calculate_iaq(&data);

    s_environment = data;

    app_state_lock();
    app_state.sensors = data;
    app_state_unlock();

    return true;
}

bool environment_read(sensor_data_t *data)
{
    if (data == NULL)
    {
        return false;
    }

    *data = s_environment;

    return s_environment.valid;
}

const sensor_data_t *environment_get(void)
{
    return &s_environment;
}
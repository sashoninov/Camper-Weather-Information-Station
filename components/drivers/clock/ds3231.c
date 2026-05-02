#include "ds3231.h"
#include "esp_log.h"
#include "i2c_bus1.h"   // <-- важно

#define DS3231_ADDR 0x68

static const char *TAG = "DS3231";

static i2c_master_dev_handle_t ds3231_dev = NULL;
extern i2c_master_bus_handle_t g_i2c1_bus;   // <-- важно

static uint8_t bcd2bin(uint8_t val)
{
    return (val & 0x0F) + ((val >> 4) * 10);
}

static uint8_t bin2bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

bool ds3231_init(void)
{
    if (ds3231_dev != NULL)
        return true;

    if (!g_i2c1_bus) {
        ESP_LOGE(TAG, "I2C1 bus not initialized");
        return false;
    }

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 100000,
    };

    if (i2c_master_bus_add_device(g_i2c1_bus, &cfg, &ds3231_dev) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add DS3231 device");
        return false;
    }

    ESP_LOGI(TAG, "DS3231 initialized");

    // Read status register
    uint8_t reg = 0x0F;
    uint8_t status = 0;

    if (i2c_master_transmit(ds3231_dev, &reg, 1, -1) == ESP_OK &&
        i2c_master_receive(ds3231_dev, &status, 1, -1) == ESP_OK)
    {
        ESP_LOGI("DS3231", "STATUS REG = 0x%02X", status);
    }

    return true;
}

bool ds3231_get_time(struct tm *out)
{
    if (!out) return false;
    if (!ds3231_init()) return false;

    uint8_t buf[7];
    uint8_t reg = 0x00;

    if (i2c_master_transmit(ds3231_dev, &reg, 1, -1) != ESP_OK)
        return false;

    if (i2c_master_receive(ds3231_dev, buf, sizeof(buf), -1) != ESP_OK)
        return false;

    ESP_LOGI("DS3231_RAW",
             "REGS: %02X %02X %02X %02X %02X %02X %02X",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

    out->tm_sec  = bcd2bin(buf[0] & 0x7F);
    out->tm_min  = bcd2bin(buf[1] & 0x7F);
    out->tm_hour = bcd2bin(buf[2] & 0x3F);
    out->tm_wday = bcd2bin(buf[3] & 0x07);
    out->tm_mday = bcd2bin(buf[4] & 0x3F);
    out->tm_mon  = bcd2bin(buf[5] & 0x1F) - 1;
    out->tm_year = bcd2bin(buf[6]) + 100;

    return true;
}

bool ds3231_set_time(const struct tm *in)
{
    if (!in) return false;
    if (!ds3231_init()) return false;

    uint8_t data[8];
    data[0] = 0x00;
    data[1] = bin2bcd(in->tm_sec);
    data[2] = bin2bcd(in->tm_min);
    data[3] = bin2bcd(in->tm_hour);
    data[4] = bin2bcd(in->tm_wday ? in->tm_wday : 1);
    data[5] = bin2bcd(in->tm_mday);
    data[6] = bin2bcd(in->tm_mon + 1);
    data[7] = bin2bcd(in->tm_year - 100);

    if (i2c_master_transmit(ds3231_dev, data, sizeof(data), -1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write DS3231 time");
        return false;
    }

    ESP_LOGI(TAG, "DS3231 time updated");
    return true;
}

void ds3231_force_set(int year, int month, int day, int hour, int min, int sec)
{
    struct tm t = {
        .tm_year = year - 1900,
        .tm_mon  = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min  = min,
        .tm_sec  = sec,
        .tm_wday = 1
    };

    if (ds3231_set_time(&t)) {
        ESP_LOGW("DS3231", "MANUAL SET DONE → %04d-%02d-%02d %02d:%02d:%02d",
                 year, month, day, hour, min, sec);
    } else {
        ESP_LOGE("DS3231", "MANUAL SET FAILED");
    }
}

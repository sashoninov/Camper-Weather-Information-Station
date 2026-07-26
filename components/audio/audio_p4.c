#include "audio_p4.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_heap_caps.h"
#include "i2c_bus.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "audio_p4";

/* -----------------------------
 * I2S PINOUT (ESP32‑P4 DevKit)
 * ----------------------------- */
#define P4_I2S_NUM        0
#define P4_I2S_MCLK       GPIO_NUM_13
#define P4_I2S_SCLK       GPIO_NUM_12
#define P4_I2S_LRCK       GPIO_NUM_10
#define P4_I2S_DOUT       GPIO_NUM_9
#define P4_I2S_DIN        GPIO_NUM_11

/* -----------------------------
 * PA CONTROL
 * ----------------------------- */
#define P4_PA_CTRL        GPIO_NUM_53

/* -----------------------------
 * AUDIO PARAMETERS
 * ----------------------------- */
#define P4_SAMPLE_RATE    44100
#define P4_MCLK_MULTIPLE  256
#define P4_MCLK_FREQ_HZ   (P4_SAMPLE_RATE * P4_MCLK_MULTIPLE)

/* -----------------------------
 * GLOBAL HANDLES
 * ----------------------------- */
static i2s_chan_handle_t tx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL;

/* -----------------------------
 * PA INIT
 * ----------------------------- */
static void audio_p4_pa_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << P4_PA_CTRL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(P4_PA_CTRL, 1);
}

/* -----------------------------
 * I2S INITIALIZATION (MASTER)
 * ----------------------------- */
static esp_err_t audio_p4_i2s_init(void)
{
    if (tx_chan) return ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(P4_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(P4_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO
        ),
        .gpio_cfg = {
            .mclk = P4_I2S_MCLK,
            .bclk = P4_I2S_SCLK,
            .ws   = P4_I2S_LRCK,
            .dout = P4_I2S_DOUT,
            .din  = P4_I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    std_cfg.clk_cfg.mclk_multiple = P4_MCLK_MULTIPLE;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = P4_I2S_NUM,
        .tx_handle = tx_chan,
        .rx_handle = NULL,
    };

    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    ESP_LOGI(TAG, "I2S init OK (MASTER, 44.1kHz, mclk_multiple=%d)", P4_MCLK_MULTIPLE);
    return ESP_OK;
}

/* -----------------------------
 * SPEAKER INITIALIZATION
 * ----------------------------- */
esp_codec_dev_handle_t audio_p4_init_speaker(void)
{
    audio_p4_pa_init();
    ESP_ERROR_CHECK(audio_p4_i2s_init());

    i2c_master_bus_handle_t bus = i2c_bus_get_handle();
    if (!bus) {
        ESP_LOGE(TAG, "I2C bus handle is NULL");
        return NULL;
    }

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    audio_codec_i2c_cfg_t i2c_cfg = {
        .bus_handle = bus,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
    };

    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = P4_PA_CTRL,
        .pa_reverted = false,
        .master_mode = false,   // ES8311 SLAVE
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .no_dac_ref = false,
        .hw_gain = gain,
    };

    const audio_codec_if_t *codec_if = es8311_codec_new(&es_cfg);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = i2s_data_if,
    };

    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);

    ESP_LOGI(TAG, "ES8311 speaker init OK (SLAVE, 44.1kHz)");
    return dev;
}

/* -----------------------------
 * PLAY 1 kHz SINE
 * ----------------------------- */
void audio_p4_play_sine_1khz(esp_codec_dev_handle_t dev)
{
    const int sample_rate = P4_SAMPLE_RATE;
    const int samples = sample_rate / 10;

    int16_t *buf = heap_caps_malloc(samples * 2 * sizeof(int16_t), MALLOC_CAP_DEFAULT);

    for (int i = 0; i < samples; i++) {
        int16_t s = (int16_t)(20000.0f * sinf(2.0f * M_PI * 1000.0f * i / sample_rate));
        buf[2 * i + 0] = s;
        buf[2 * i + 1] = s;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = sample_rate,
        .bits_per_sample = 16,
        .channel = 2,
    };

    esp_codec_dev_open(dev, &fs);
    esp_codec_dev_set_out_mute(dev, false);
    esp_codec_dev_set_out_vol(dev, 90);

    esp_codec_dev_write(dev, buf, samples * 2 * sizeof(int16_t));

    esp_codec_dev_close(dev);
    free(buf);
}

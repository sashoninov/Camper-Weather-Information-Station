#pragma once

#include "esp_codec_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ES8311 speaker output
 * @return codec device handle
 */
esp_codec_dev_handle_t audio_p4_init_speaker(void);

/**
 * @brief Play 1 kHz sine wave (test tone)
 */
void audio_p4_play_sine_1khz(esp_codec_dev_handle_t dev);

#ifdef __cplusplus
}
#endif

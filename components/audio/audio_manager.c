#include "audio_manager.h"
#include "audio_events.h"
#include "audio_p4.h"
#include <stdint.h>

#include "esp_log.h"
#include "esp_codec_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdio.h>
#include <string.h>

#define TAG "audio_manager"

typedef struct {
    char path[128];
    audio_priority_t prio;
} audio_msg_t;

static QueueHandle_t audio_queue = NULL;
static TaskHandle_t audio_task_handle = NULL;

static esp_codec_dev_handle_t audio_dev = NULL;
static bool audio_busy = false;

/* ---------------------------------------------------------
 * WAV STREAMING USING esp_codec_dev_write()
 * --------------------------------------------------------- */
static bool play_wav_file(const char *path)
{
    if (!path) {
        ESP_LOGE(TAG, "NULL path");
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open WAV: %s", path);
        return false;
    }

    // Skip WAV header (44 bytes)
    fseek(f, 44, SEEK_SET);

    uint8_t buffer[1024];
    size_t bytes_read;

    ESP_LOGI(TAG, "Playing: %s", path);

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        esp_codec_dev_write(audio_dev, buffer, bytes_read);
    }

    fclose(f);
    return true;
}

/* ---------------------------------------------------------
 * AUDIO TASK
 * --------------------------------------------------------- */
static void audio_task(void *arg)
{
    audio_msg_t msg;

    while (1) {
        if (xQueueReceive(audio_queue, &msg, portMAX_DELAY)) {
            audio_busy = true;
            play_wav_file(msg.path);
            audio_busy = false;
        }
    }
}

/* ---------------------------------------------------------
 * PUBLIC API
 * --------------------------------------------------------- */
void audio_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Audio Manager...");

    audio_dev = audio_p4_init_speaker();
    if (!audio_dev) {
        ESP_LOGE(TAG, "Failed to init audio_p4 speaker!");
        return;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 2,
    };

    esp_codec_dev_open(audio_dev, &fs);
    esp_codec_dev_set_out_mute(audio_dev, false);
    esp_codec_dev_set_out_vol(audio_dev, 90);

    audio_queue = xQueueCreate(8, sizeof(audio_msg_t));

    xTaskCreatePinnedToCore(
        audio_task,
        "audio_task",
        4096,
        NULL,
        5,
        &audio_task_handle,
        1
    );

    ESP_LOGI(TAG, "Audio Manager ready");
}

bool audio_play_file(const char *path)
{
    if (!path) return false;

    audio_msg_t msg = {0};
    strncpy(msg.path, path, sizeof(msg.path) - 1);
    msg.prio = AUDIO_PRIORITY_NORMAL;

    return xQueueSend(audio_queue, &msg, 0) == pdTRUE;
}

bool audio_play_event(audio_event_t event)
{
    const char *path = audio_event_get_path(event);
    return audio_play_file(path);
}

bool audio_play_event_prio(audio_event_t event, audio_priority_t prio)
{
    const char *path = audio_event_get_path(event);
    if (!path) return false;

    audio_msg_t msg = {0};
    strncpy(msg.path, path, sizeof(msg.path) - 1);
    msg.prio = prio;

    if (prio == AUDIO_PRIORITY_HIGH) {
        xQueueReset(audio_queue);
    }

    return xQueueSend(audio_queue, &msg, 0) == pdTRUE;
}

bool audio_is_busy(void)
{
    return audio_busy;
}

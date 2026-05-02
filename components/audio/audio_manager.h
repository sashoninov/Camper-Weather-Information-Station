#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "audio_events.h"

typedef enum {
    AUDIO_PRIORITY_LOW = 0,
    AUDIO_PRIORITY_NORMAL,
    AUDIO_PRIORITY_HIGH
} audio_priority_t;

void audio_manager_init(void);
bool audio_play_event(audio_event_t event);
bool audio_play_file(const char *path);
bool audio_play_event_prio(audio_event_t event, audio_priority_t prio);
bool audio_is_busy(void);

#ifdef __cplusplus
}
#endif

idf_component_register(
    SRCS "audio_manager.c" "audio_events.c" "audio_p4.c"
    INCLUDE_DIRS "."
    REQUIRES esp_codec_dev driver drivers
)

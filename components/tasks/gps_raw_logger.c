#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define UART_PORT UART_NUM_1
#define TX_PIN 21
#define RX_PIN 22
#define BUF_SIZE 1024

static const char *TAG = "GPS_RAW";

static char line[256];
static int line_pos = 0;

void gps_raw_logger_task(void *arg)
{
    uint8_t buf[64];

    ESP_LOGW(TAG, "GPS RAW LOGGER STARTED");

    while (1)
    {
        int len = uart_read_bytes(UART_PORT, buf, sizeof(buf), 200 / portTICK_PERIOD_MS);
        if (len <= 0) continue;

        for (int i = 0; i < len; i++)
        {
            char c = buf[i];

            if (c == '\n')
            {
                line[line_pos] = 0;
                line_pos = 0;

                // Логваме ВСЕКИ ред, който започва с $
                if (line[0] == '$')
                    ESP_LOGW(TAG, "%s", line);

                continue;
            }

            if (line_pos < sizeof(line) - 1)
                line[line_pos++] = c;
        }
    }
}

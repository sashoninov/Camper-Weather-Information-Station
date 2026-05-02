#include "victron_driver.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define UART_PORT UART_NUM_2
#define UART_RX   45
#define BUF_SIZE 256

static const char *TAG = "VICTRON";

void victron_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);

    // Само RX, без TX!
    uart_set_pin(UART_PORT,
                 UART_PIN_NO_CHANGE,
                 UART_RX,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "Victron UART2 initialized (RX=%d)", UART_RX);
}

bool victron_read_line(char *out, int max_len)
{
    static char line[128];
    static int idx = 0;

    uint8_t c;
    int len = uart_read_bytes(UART_PORT, &c, 1, pdMS_TO_TICKS(50));

    if (len <= 0) return false;

    if (c == '\n') {
        line[idx] = 0;
        strncpy(out, line, max_len);
        idx = 0;
        return true;
    }

    if (c != '\r' && idx < sizeof(line) - 1) {
        line[idx++] = c;
    }

    return false;
}

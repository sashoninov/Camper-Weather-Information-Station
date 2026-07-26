#include "gps_task.h"
#include "gps.h"
#include "nmea.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>


#define GPS_UART_NUM           UART_NUM_1
#define GPS_UART_BAUDRATE      115200

#define GPS_UART_TX_PIN UART_PIN_NO_CHANGE
#define GPS_UART_RX_PIN   33      

#define GPS_RX_BUFFER_SIZE     1024
#define GPS_LINE_MAX_LEN       128

static const char *TAG = "GPS_TASK";
static int64_t last_log_ms = 0;

static void gps_task(void *arg)
{
    uint8_t rx[64];

    char line[GPS_LINE_MAX_LEN];
    size_t pos = 0;

    while (1)
    {
		int len = uart_read_bytes(
			GPS_UART_NUM,
			rx,
			sizeof(rx),
			pdMS_TO_TICKS(100));
		
		if (len <= 0)
			continue;
		
		

        for (int i = 0; i < len; i++)
        {
            char c = (char)rx[i];

            if (c == '\r')
                continue;

            if (c == '\n')
            {
                if (pos > 0)
                {
                    line[pos] = '\0';

                    nmea_parse(line);

                    //* Печатаме само след RMC */
                    if (strncmp(line, "$GNRMC", 6) == 0 ||
                        strncmp(line, "$GPRMC", 6) == 0)
                    {
                        gps_data_t gps;
					
						if (gps_get_data(&gps))
						{
							int64_t now_ms = esp_timer_get_time() / 1000;
						
							if ((now_ms - last_log_ms) >= 60000)   // веднъж на 60 секунди
							{
								last_log_ms = now_ms;
						
								ESP_LOGI(TAG,
										"Fix:%d Sat:%u HDOP:%.1f Alt:%.1f Lat:%.6f Lon:%.6f",
										gps.fix,
										gps.satellites,
										gps.hdop,
										gps.altitude,
										gps.latitude,
										gps.longitude);
							}
						}
                    }
                }

                pos = 0;
                continue;
            }

            if (pos < GPS_LINE_MAX_LEN - 1)
            {
                line[pos++] = c;
            }
            else
            {
                /* Overflow -> discard current line */
                pos = 0;
            }
        }
    }
}

bool gps_task_start(void)
{
    uart_config_t cfg = {
        .baud_rate = GPS_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        GPS_UART_NUM,
        GPS_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0));

    ESP_ERROR_CHECK(uart_param_config(
        GPS_UART_NUM,
        &cfg));

    ESP_ERROR_CHECK(uart_set_pin(
        GPS_UART_NUM,
        GPS_UART_TX_PIN,
        GPS_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
		
	ESP_LOGI(TAG, "UART=%d", GPS_UART_NUM);
	ESP_LOGI(TAG, "RX pin=%d", GPS_UART_RX_PIN);
	ESP_LOGI(TAG, "Baud=%d", GPS_UART_BAUDRATE);	

    BaseType_t ok = xTaskCreate(
        gps_task,
        "gps_task",
        4096,
        NULL,
        5,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create task");
        return false;
    }

    ESP_LOGI(TAG, "GPS task started");

    return true;
}
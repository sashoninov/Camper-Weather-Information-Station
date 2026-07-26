#include "http_gateway.h"
#include "sim_a7670.h"


#include "wifi.h"
#include "app_state.h"

#include "esp_http_client.h"
#include "network_manager.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>




void http_gateway_init(void)
{
    // Няма нужда от глобален mutex
}

int http_get(const char *url, char *buf, size_t buf_len)
{
    if (!url || !buf || buf_len == 0)
        return -1;

    // ============================
    // GSM MODE
    // ============================
    if (network_is_gsm())
    {
		if (!gsm_status.ready)
		{
			printf("⚠️ GSM not ready → HTTP skipped\n");
			return -1;
		}

        int n = sim_a7670_http_get(url, buf, buf_len);
        if (n > 0)
        {
            buf[n] = '\0';
            return n;
        }

        return -1;
    }

    // ============================
    // WIFI MODE
    // ============================
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 8000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client)
        return -1;

    if (esp_http_client_open(client, 0) != ESP_OK)
    {
        esp_http_client_cleanup(client);
        return -1;
    }

    int n = esp_http_client_read(client, buf, buf_len - 1);
    if (n > 0)
        buf[n] = '\0';
    else
        buf[0] = '\0';

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return n;
}

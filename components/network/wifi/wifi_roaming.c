#include "wifi_roaming.h"
#include "wifi.h"
#include "wifi_storage.h"
#include "app_state.h"

#include "lwip/api.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

volatile bool roaming_enabled = true;

void wifi_roaming_pause(void) { roaming_enabled = false; }
void wifi_roaming_resume(void) { roaming_enabled = true; }

static bool api_test_ok(void)
{
    struct netconn *conn = netconn_new(NETCONN_TCP);
    if (!conn) return false;

    ip_addr_t addr;
    if (netconn_gethostbyname("connectivitycheck.gstatic.com", &addr) != ERR_OK) {
        netconn_delete(conn);
        return false;
    }

    if (netconn_connect(conn, &addr, 80) != ERR_OK) {
        netconn_delete(conn);
        return false;
    }

    const char *req =
        "GET /generate_204 HTTP/1.1\r\n"
        "Host: connectivitycheck.gstatic.com\r\n"
        "Connection: close\r\n\r\n";

    netconn_write(conn, req, strlen(req), NETCONN_COPY);

    struct netbuf *buf;
    if (netconn_recv(conn, &buf) != ERR_OK) {
        netconn_close(conn);
        netconn_delete(conn);
        return false;
    }

    char *data;
    u16_t len;
    netbuf_data(buf, (void**)&data, &len);

    bool ok = (len > 0 && strstr(data, "204") != NULL);

    netbuf_delete(buf);
    netconn_close(conn);
    netconn_delete(conn);

    return ok;
}

static void roaming_task(void *arg)
{
    char saved_ssid[33] = {0};
    char saved_pass[64] = {0};

    wifi_load_credentials(saved_ssid, saved_pass);

    static wifi_ap_record_t recs[20];

    while (1)
    {
        if (!roaming_enabled) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (wifi_is_connected()) {
            if (api_test_ok()) {
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));

        wifi_scan_config_t cfg = {0};
        esp_wifi_scan_start(&cfg, true);

        uint16_t n = 20;
        esp_wifi_scan_get_ap_records(&n, recs);

        bool saved_found = false;

        for (int i = 0; i < n; i++) {
            if (strcmp((char*)recs[i].ssid, saved_ssid) == 0) {
                saved_found = true;
                break;
            }
        }

        if (saved_found) {
            wifi_set_credentials(saved_ssid, saved_pass);
            vTaskDelay(pdMS_TO_TICKS(8000));
            continue;
        }

        for (int i = 0; i < n; i++) {

            if (recs[i].authmode != WIFI_AUTH_OPEN)
                continue;

            if (wifi_test_connection((char*)recs[i].ssid, "")) {

                if (api_test_ok()) {
                    break;
                }
            }
        }
    }
}

void wifi_roaming_start(void)
{
    xTaskCreate(roaming_task, "wifi_roaming", 8192, NULL, 5, NULL);
}

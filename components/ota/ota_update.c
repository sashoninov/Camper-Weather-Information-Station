#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "OTA";

// Глобален handle, за да не стартираме сървъра втори път
static httpd_handle_t ota_server = NULL;

// ============================================================
//  OTA POST HANDLER — multipart/form-data safe
// ============================================================

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int remaining = req->content_len;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t update_handle = 0;

    ESP_LOGI(TAG, "Starting OTA update...");
    ESP_ERROR_CHECK(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle));

    bool data_started = false;

    while (remaining > 0) {

        int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (recv_len <= 0) {
            ESP_LOGE(TAG, "OTA receive error");
            return ESP_FAIL;
        }

        // ⭐ Пропускаме multipart header-а
        if (!data_started) {
            char *start = strstr(buf, "\r\n\r\n");
            if (start) {
                start += 4; // skip CRLFCRLF
                int header_len = start - buf;
                int payload_len = recv_len - header_len;

                if (payload_len > 0) {
                    ESP_ERROR_CHECK(esp_ota_write(update_handle, start, payload_len));
                }

                data_started = true;
            }
        } else {
            ESP_ERROR_CHECK(esp_ota_write(update_handle, buf, recv_len));
        }

        remaining -= recv_len;
    }

    ESP_ERROR_CHECK(esp_ota_end(update_handle));
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(update_partition));

    httpd_resp_sendstr(req, "OK – Rebooting...");
    ESP_LOGI(TAG, "OTA update complete. Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

// ============================================================
//  OTA GET HANDLER — HTML upload form
// ============================================================

static esp_err_t ota_get_handler(httpd_req_t *req)
{
    const char *html =
        "<form method='POST' enctype='multipart/form-data'>"
        "<input type='file' name='firmware'>"
        "<input type='submit' value='Update'>"
        "</form>";

    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

// ============================================================
//  OTA SERVER START
// ============================================================

void ota_start_server(void)
{
    // ⭐ Предпазване от второ стартиране
    if (ota_server != NULL) {
        ESP_LOGW(TAG, "OTA server already running");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t err = httpd_start(&ota_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OTA server: %s", esp_err_to_name(err));
        return;
    }

    httpd_uri_t ota_get = {
        .uri = "/update",
        .method = HTTP_GET,
        .handler = ota_get_handler
    };

    httpd_uri_t ota_post = {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = ota_post_handler
    };

    httpd_register_uri_handler(ota_server, &ota_get);
    httpd_register_uri_handler(ota_server, &ota_post);

    ESP_LOGI(TAG, "OTA server started. Open http://<device-ip>/update");
}

// ============================================================
//  OPTIONAL: STOP SERVER (ако някога ти потрябва)
// ============================================================

void ota_stop_server(void)
{
    if (ota_server) {
        httpd_stop(ota_server);
        ota_server = NULL;
        ESP_LOGI(TAG, "OTA server stopped");
    }
}

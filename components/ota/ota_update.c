#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdarg.h>

#include "cJSON.h"
#include "status.h"

static const char *TAG = "OTA";
static httpd_handle_t ota_server = NULL;

/* ============================================================
   SYSTEM LOG BUFFER (100 lines)
   ============================================================ */

#define LOG_LINES 100
#define LOG_LINE_LEN 256

static char log_buffer[LOG_LINES][LOG_LINE_LEN];
static int log_index = 0;
static SemaphoreHandle_t log_mutex;

/* Append log line */
static void log_append(const char *msg)
{
    if (!log_mutex) return;
    xSemaphoreTake(log_mutex, portMAX_DELAY);

    snprintf(log_buffer[log_index], LOG_LINE_LEN, "%s", msg);
    log_index = (log_index + 1) % LOG_LINES;

    xSemaphoreGive(log_mutex);
}

/* vprintf hook */
static int log_vprintf_hook(const char *fmt, va_list args)
{
    char buf[LOG_LINE_LEN];
    vsnprintf(buf, sizeof(buf), fmt, args);

    log_append(buf);

    return vprintf(fmt, args);
}

/* ============================================================
   /log endpoint
   ============================================================ */
static esp_err_t log_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateArray();

    xSemaphoreTake(log_mutex, portMAX_DELAY);

    int idx = log_index;
    for (int i = 0; i < LOG_LINES; i++) {
        int pos = (idx + i) % LOG_LINES;
        if (strlen(log_buffer[pos]) > 0)
            cJSON_AddItemToArray(root, cJSON_CreateString(log_buffer[pos]));
    }

    xSemaphoreGive(log_mutex);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    httpd_resp_sendstr(req, json);
    free(json);
    return ESP_OK;
}

/* ============================================================
   OTA POST HANDLER
   ============================================================ */
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

        if (!data_started) {
            char *start = strstr(buf, "\r\n\r\n");
            if (start) {
                start += 4;
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

/* ============================================================
   OTA GET HANDLER
   ============================================================ */
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

/* ============================================================
   STATUS JSON HANDLER
   ============================================================ */
static esp_err_t status_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "starter_v", g_status.starter_v);
    cJSON_AddNumberToObject(root, "camper_v",  g_status.camper_v);
    cJSON_AddNumberToObject(root, "camper_a",  g_status.camper_a);
    cJSON_AddNumberToObject(root, "pitch",     g_status.pitch);
    cJSON_AddNumberToObject(root, "roll",      g_status.roll);

    uint32_t uptime = xTaskGetTickCount() / configTICK_RATE_HZ;
    cJSON_AddNumberToObject(root, "uptime", uptime);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    free(json);
    return ESP_OK;
}

/* ============================================================
   MAIN PANEL HANDLER (HTML + CSS + JS)
   ============================================================ */
static esp_err_t panel_get_handler(httpd_req_t *req)
{
    const char *html =
"<!DOCTYPE html>"
"<html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Camper Dashboard</title>"
"<style>"
"body{margin:0;padding:0;background:#0d0d0d;color:#eee;font-family:sans-serif;}"
".container{max-width:650px;margin:10px auto;padding:10px;background:#1a1a1a;border-radius:12px;}"

"h2{margin-top:0;color:#4cafef;text-align:left;display:flex;justify-content:space-between;align-items:center;font-size:22px;}"
".ota-btn{background:#4cafef;color:#000;padding:10px 18px;border-radius:8px;text-decoration:none;font-weight:bold;font-size:18px;}"

".section{margin:15px 0;padding:12px;background:#111;border-radius:10px;}"
".label{color:#888;font-size:14px;}"
".value{font-size:20px;margin-top:4px;}"

/* LEVEL UI */
".level-container{display:flex;flex-direction:column;gap:20px;align-items:center;}"
".level-block{display:flex;flex-direction:column;align-items:center;}"

/* Bars */
".level-vert{width:50px;height:180px;border:1.5px solid #4cafef;border-radius:10px;position:relative;background:#0f0f0f;}"
".level-horiz{width:180px;height:50px;border:1.5px solid #4cafef;border-radius:10px;position:relative;background:#0f0f0f;}"

/* Indicator */
".bubble{width:22px;height:22px;border-radius:50%;position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);transition:0.08s linear;}"

/* Lines */
".tick{position:absolute;left:0;width:100%;height:1px;background:#4cafef55;}"
".tick-strong{height:2px;background:#4cafefaa;}"

".tick-horiz{position:absolute;top:0;height:100%;width:1px;background:#4cafef55;}"
".tick-horiz-strong{width:2px;background:#4cafefaa;}"

".angle-text{margin-top:6px;font-size:18px;color:#4cafef;}"

/* SYSTEM LOG */
"#logbox{height:160px;overflow-y:scroll;background:#000;border:1px solid #333;padding:8px;font-family:monospace;color:#55aa55;font-size:12px;white-space:pre-wrap;}"
"</style>"
"</head><body>"

"<div class='container'>"
"<h2>Camper Dashboard <a class='ota-btn' href='/update'>OTA Update</a></h2>"

/* STARTER BATTERY */
"<div class='section'>"
"<div class='label'>Starter Battery</div>"
"<div class='value' id='starter_v'>--</div>"
"</div>"

/* CAMPER BATTERY */
"<div class='section'>"
"<div class='label'>Camper Battery</div>"
"<div class='value' id='camper_v'>--</div>"
"<div class='value' id='camper_a'>--</div>"
"</div>"

/* LEVEL SECTION */
"<div class='section'>"
"<div class='label'>Level (MPU6050)</div>"

"<div class='level-container'>"

/* PITCH BAR */
"<div class='level-block'>"
"<div class='level-vert'>"
"<div class='tick tick-strong' style='top:50%;'></div>"
"<div class='tick' style='top:25%;'></div>"
"<div class='tick' style='top:37.5%;'></div>"
"<div class='tick' style='top:62.5%;'></div>"
"<div class='tick' style='top:75%;'></div>"
"<div id='bubble_pitch' class='bubble'></div>"
"</div>"
"<div id='pitch_text' class='angle-text'>0°</div>"
"</div>"

/* ROLL BAR */
"<div class='level-block'>"
"<div class='level-horiz'>"
"<div class='tick-horiz tick-horiz-strong' style='left:50%;'></div>"
"<div class='tick-horiz' style='left:25%;'></div>"
"<div class='tick-horiz' style='left:37.5%;'></div>"
"<div class='tick-horiz' style='left:62.5%;'></div>"
"<div class='tick-horiz' style='left:75%;'></div>"
"<div id='bubble_roll' class='bubble'></div>"
"</div>"
"<div id='roll_text' class='angle-text'>0°</div>"
"</div>"

"</div></div>"

/* SYSTEM LOG */
"<div class='section'>"
"<div class='label'>System Log</div>"
"<div id='logbox'></div>"
"</div>"

"</div>"

"<script>"
"function clamp(v,min,max){return Math.max(min,Math.min(max,v));}"
"function getColor(a){a=Math.abs(a);if(a<3)return'#4CAF50';if(a<7)return'#FFC107';return'#F44336';}"

"async function refresh(){"
" try{"
"  const r=await fetch('/status');"
"  const j=await r.json();"

"  document.getElementById('starter_v').textContent=j.starter_v.toFixed(2)+' V';"
"  document.getElementById('camper_v').textContent=j.camper_v.toFixed(2)+' V';"
"  let a=j.camper_a;"
"  document.getElementById('camper_a').textContent=(a>=0?'+':'')+a.toFixed(2)+' A';"

"  updateLevel(j.roll,j.pitch);"

"  let up=j.uptime;"
"  let h=Math.floor(up/3600);"
"  let m=Math.floor((up%3600)/60);"
"  let s=up%60;"
"  document.getElementById('logbox').title='Uptime: '+h+'h '+m+'m '+s+'s';"

" }catch(e){console.log(e);} }"

"async function refreshLog(){"
" try{"
"  const r=await fetch('/log');"
"  const j=await r.json();"
"  let out='';"
"  for(let i=0;i<j.length;i++){"
"    let line=j[i];"
"    if(line.includes('E (')) line='<span style=\"color:#ff5555\">'+line+'</span>';"
"    else if(line.includes('W (')) line='<span style=\"color:#ffaa00\">'+line+'</span>';"
"    else if(line.includes('I (')) line='<span style=\"color:#55ff55\">'+line+'</span>';"
"    out+=line;"
"  }"
"  document.getElementById('logbox').innerHTML=out;"
"  let lb=document.getElementById('logbox');"
"  lb.scrollTop=lb.scrollHeight;"
" }catch(e){console.log(e);} }"

"function updateLevel(pitch,roll){"
" const bp=document.getElementById('bubble_pitch');"
" const br=document.getElementById('bubble_roll');"
" const tp=document.getElementById('pitch_text');"
" const tr=document.getElementById('roll_text');"

" const max=80;"

" const y=clamp((pitch/20)*max,-max,max);"
" const x=clamp((roll /20)*max,-max,max);"

" bp.style.transform='translate(-50%, calc(-50% + '+y+'px))';"
" br.style.transform='translate(calc(-50% + '+x+'px), -50%)';"

" const cp=getColor(pitch);"
" const cr=getColor(roll);"

" bp.style.background=cp;"
" br.style.background=cr;"

" tp.textContent=pitch.toFixed(1)+'°';"
" tr.textContent=roll.toFixed(1)+'°';"

" tp.style.color=cp;"
" tr.style.color=cr;"
"}"

"setInterval(refresh,500);"
"setInterval(refreshLog,1000);"
"refresh();"
"refreshLog();"
"</script>"

"</body></html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

/* ============================================================
   OTA SERVER START
   ============================================================ */
void ota_start_server(void)
{
    if (ota_server != NULL) {
        ESP_LOGW(TAG, "OTA server already running");
        return;
    }

    log_mutex = xSemaphoreCreateMutex();
    esp_log_set_vprintf(log_vprintf_hook);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&ota_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OTA server");
        return;
    }

    httpd_uri_t ota_get = { .uri="/update", .method=HTTP_GET, .handler=ota_get_handler };
    httpd_uri_t ota_post= { .uri="/update", .method=HTTP_POST,.handler=ota_post_handler };
    httpd_uri_t status_get={ .uri="/status", .method=HTTP_GET, .handler=status_get_handler };
    httpd_uri_t panel_get={ .uri="/", .method=HTTP_GET, .handler=panel_get_handler };
    httpd_uri_t log_get={ .uri="/log", .method=HTTP_GET, .handler=log_get_handler };

    httpd_register_uri_handler(ota_server,&ota_get);
    httpd_register_uri_handler(ota_server,&ota_post);
    httpd_register_uri_handler(ota_server,&status_get);
    httpd_register_uri_handler(ota_server,&panel_get);
    httpd_register_uri_handler(ota_server,&log_get);

    ESP_LOGI(TAG,"OTA server started. Open http://<device-ip>/");
}

/* ============================================================
   STOP SERVER
   ============================================================ */
void ota_stop_server(void)
{
    if (ota_server) {
        httpd_stop(ota_server);
        ota_server = NULL;
        ESP_LOGI(TAG, "OTA server stopped");
    }
}

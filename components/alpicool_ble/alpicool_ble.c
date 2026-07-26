#include "alpicool_ble.h"
#include "alpicool_packet.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"
#include "os/os_mbuf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "power_manager.h"

#include <string.h>



static int gap_event(struct ble_gap_event *event, void *arg);

static const char *TAG = "ALPICOOL_BLE";

static uint16_t conn_handle   = BLE_HS_CONN_HANDLE_NONE;
static uint8_t  own_addr_type = 0;

/* TX = write-no-rsp characteristic, NOTIFY = notify characteristic */
static uint16_t tx_handle     = 0x0004;
static uint16_t notify_handle = 0x0006;

static const uint8_t ALPICOOL_QUERY[] = {
    0xFE, 0xFE, 0x03, 0x01, 0x02, 0x00
};

static esp_timer_handle_t reconnect_timer;
static esp_timer_handle_t disconnect_timer;

/* ============================
 * COMMAND QUEUE
 * ============================ */
typedef struct {
    uint8_t data[32];
    uint16_t len;
} alpicool_cmd_t;

static QueueHandle_t s_cmd_queue = NULL;

/* ============================
 * HUMAN-READABLE HELPERS
 * ============================ */
static const char* mode_str(uint8_t m) { return (m == 1) ? "ECO" : "MAX"; }
static const char* power_str(uint8_t p) { return (p == 1) ? "ON" : "OFF"; }
static const char* protect_str(uint8_t p) {
    switch (p) { case 0: return "L"; case 1: return "M"; case 2: return "H"; }
    return "?";
}
static const char* lock_str(uint8_t l) { return (l == 1) ? "LOCKED" : "UNLOCKED"; }

/* ============================
 * PARSER
 * ============================ */
static bool alpicool_parse(const uint8_t *d,
                           int len,
                           alpicool_status_t *status)
{
    if (len < 20)
        return false;

    if (d[0] != 0xFE || d[1] != 0xFE)
        return false;

    if (d[3] == 0x01) {
        const uint8_t *p = &d[4];

        status->lock         = p[0];
        status->power        = p[1];
        status->mode         = p[2];
        status->batt_protect = p[3];
        status->set_temp     = (int8_t)p[4];

        status->real_temp    = (int8_t)p[0x0E];
        status->battery      = p[0x0F];

        uint8_t vi = p[16];
        uint8_t vd = p[17];
        status->voltage = vi + (vd / 10.0f);

        ESP_LOGI(TAG,
            "STATUS: SET=%d REAL=%d MODE=%s POWER=%s BAT=%u%% PROT=%s LOCK=%s VOLT=%.1fV",
            status->set_temp,
            status->real_temp,
            mode_str(status->mode),
            power_str(status->power),
            status->battery,
            protect_str(status->batt_protect),
            lock_str(status->lock),
            status->voltage
        );

        return true;
    }

    return false;
}

/* ============================
 * LOW-LEVEL SEND
 * ============================ */
static int alpicool_send(uint16_t handle, const uint8_t *data, uint16_t len)
{
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "No connection, cannot send");
        return BLE_HS_ENOTCONN;
    }

    ESP_LOGI(TAG, "SEND (len=%u)", len);
    ESP_LOG_BUFFER_HEX(TAG, data, len);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) return BLE_HS_ENOMEM;

    int rc = ble_gattc_write_no_rsp(conn_handle, handle, om);
    ESP_LOGI(TAG, "write rc=%d", rc);
    return rc;
}

/* ============================
 * COMMAND TASK (THROTTLING)
 * ============================ */
static void alpicool_cmd_task(void *arg)
{
    alpicool_cmd_t cmd;

    while (1) {
        if (xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
			
			while (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
				vTaskDelay(pdMS_TO_TICKS(50));
			}

            alpicool_send(tx_handle, cmd.data, cmd.len);

            uint32_t delay_ms = 200;
            if (cmd.len <= 6)      delay_ms = 150;
            else if (cmd.len == 7) delay_ms = 250;
            else                   delay_ms = 400;

            vTaskDelay(pdMS_TO_TICKS(delay_ms));
			
        }
    }
}

/* ============================
 * DISCONNECT TIMER
 * ============================ */
static void disconnect_cb(void *arg)
{
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "FORCED DISCONNECT (not used)");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

void alpicool_ble_disconnect(void)
{
    ESP_LOGI(TAG, "========== DISCONNECT REQUEST ==========");

    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "No active connection");
        return;
    }

    ESP_LOGI(TAG, "Disconnecting conn_handle=%u", conn_handle);

    int rc = ble_gap_terminate(
        conn_handle,
        BLE_ERR_REM_USER_CONN_TERM);

    ESP_LOGI(TAG, "ble_gap_terminate() rc=%d", rc);
}
/* ============================
 * RECONNECT TIMER
 * ============================ */
static void reconnect_cb(void *arg)
{
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE)
        return;

    ESP_LOGI(TAG, "Scanning for NL50...");

    struct ble_gap_disc_params p = {0};
    p.passive = 0;
    p.itvl    = 0x20;
    p.window  = 0x10;

    int rc = ble_gap_disc(own_addr_type, 5000, &p, gap_event, NULL);
    if (rc != 0) {
        ESP_LOGW(TAG, "ble_gap_disc failed rc=%d", rc);
    }
}

/* ============================
 * SHORT TEMP COMMAND
 * ============================ */
int alpicool_ble_send_temp_short(int8_t t)
{
    uint8_t cmd[7];
    cmd[0] = 0xFE;
    cmd[1] = 0xFE;
    cmd[2] = 0x04;
    cmd[3] = 0x05;
    cmd[4] = (uint8_t)t;   // two's complement

    if (t >= 0 || t < -5)
        cmd[5] = 0x02;
    else
        cmd[5] = 0x03;

    cmd[6] = cmd[4] + 0x05;

    ESP_LOGI(TAG, "CMD: SET_TEMP(short)=%d", t);
    ESP_LOG_BUFFER_HEX(TAG, cmd, sizeof(cmd));

    alpicool_cmd_t c = {0};
    memcpy(c.data, cmd, sizeof(cmd));
    c.len = sizeof(cmd);

    return xQueueSend(s_cmd_queue, &c, 0) == pdTRUE ? 0 : -1;
}

/* ============================
 * UNIVERSAL COMMAND
 * ============================ */
int alpicool_ble_send_universal(int8_t temp,
                                uint8_t mode,
                                uint8_t power,
                                uint8_t lock,
                                uint8_t batt)
{
    uint8_t cmd[20];

	alpicool_status_t st = {0};
	
	st.set_temp     = temp;
	st.mode         = mode;
	st.power        = power;
	st.lock         = lock;
	st.batt_protect = batt;
	
	alpicool_packet_build(cmd, &st);

    ESP_LOGI(TAG, "CMD: UNIVERSAL T=%d M=%u P=%u L=%u B=%u",
             temp, mode, power, lock, batt);

    ESP_LOG_BUFFER_HEX(TAG, cmd, sizeof(cmd));

    alpicool_cmd_t c = {0};
    memcpy(c.data, cmd, sizeof(cmd));
    c.len = sizeof(cmd);

    return xQueueSend(s_cmd_queue, &c, 0) == pdTRUE ? 0 : -1;
}
/* ============================
 * GAP EVENT
 * ============================ */
static int gap_event(struct ble_gap_event *event, void *arg)
{
	ESP_LOGI(TAG, "GAP EVENT type=%d", event->type);
	
    switch (event->type) {

    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields f = {0};
        ble_hs_adv_parse_fields(&f, event->disc.data, event->disc.length_data);

        char name[32] = {0};
        if (f.name && f.name_len < sizeof(name)) {
            memcpy(name, f.name, f.name_len);
            name[f.name_len] = 0;
        }

        if (strcmp(name, "AK1-2BFF05128442") == 0) {
            ESP_LOGI(TAG, "Found NL50, connecting...");

            ble_gap_disc_cancel();

            struct ble_gap_conn_params cp = {
                .scan_itvl = 0x20,
                .scan_window = 0x10,
                .itvl_min = 24,
                .itvl_max = 40,
                .latency = 0,
                .supervision_timeout = 400,
            };

            int rc = ble_gap_connect(own_addr_type,
                                     &event->disc.addr,
                                     30000,
                                     &cp,
                                     gap_event,
                                     NULL);
            if (rc != 0) {
                ESP_LOGW(TAG, "ble_gap_connect failed rc=%d", rc);
            }
        }
        return 0;
    }

    case BLE_GAP_EVENT_CONNECT:
	
	    ESP_LOGI(TAG,
             "CONNECT EVENT status=%d conn_handle=%d",
             event->connect.status,
             event->connect.conn_handle);
			 
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "Connected (conn_handle=%u)", event->connect.conn_handle);
            conn_handle = event->connect.conn_handle;

            uint16_t cccd_handle = notify_handle + 1;
            uint8_t  cccd_value[2] = {0x01, 0x00};
            ble_gattc_write_flat(conn_handle, cccd_handle,
                                 cccd_value, sizeof(cccd_value),
                                 NULL, NULL);

            //ESP_LOGI(TAG, "SENDING INITIAL QUERY");
            //alpicool_send(tx_handle, ALPICOOL_QUERY, sizeof(ALPICOOL_QUERY));
			ESP_LOGI(TAG, "CONNECTED -> QUERY VIA QUEUE");
			alpicool_ble_query_status();

        } else {
            ESP_LOGW(TAG, "Connect failed (status=%d)", event->connect.status);
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            esp_timer_stop(reconnect_timer);
            esp_timer_start_once(reconnect_timer, 5000000);
        }
        return 0;

	case BLE_GAP_EVENT_NOTIFY_RX: {
	
		int len = OS_MBUF_PKTLEN(event->notify_rx.om);
	
		alpicool_status_t status = {0};
	
		if (alpicool_parse(event->notify_rx.om->om_data, len, &status)) {
	
			alpicool_status_received(&status);
	
			alpicool_ble_disconnect();
		}
	
		return 0;
	}

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGW(TAG,
				 "Disconnected."
				 " reason=%d"
				 " conn_handle=%u",
				 event->disconnect.reason,
				 conn_handle);
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
		power_manager_ble_disconnected();

        return 0;
		
		case BLE_GAP_EVENT_DISC_COMPLETE:

			ESP_LOGI(TAG, "Scan complete");
		
			return 0;		

    default:
        return 0;
    }
}

/* ============================
 * SYNC CALLBACK
 * ============================ */
void ble_on_sync(void)
{
    ESP_LOGI(TAG, "BLE synced");

    ble_hs_id_infer_auto(0, &own_addr_type);

    const esp_timer_create_args_t t = {
        .callback = reconnect_cb,
        .name     = "reconnect"
    };
    esp_timer_create(&t, &reconnect_timer);

    const esp_timer_create_args_t dt = {
        .callback = disconnect_cb,
        .name     = "disc_timer"
    };
    esp_timer_create(&dt, &disconnect_timer);

    //esp_timer_start_once(reconnect_timer, 1000000);

    if (!s_cmd_queue) {
        s_cmd_queue = xQueueCreate(16, sizeof(alpicool_cmd_t));
        xTaskCreate(alpicool_cmd_task, "alpicool_cmd_task", 4096, NULL, 5, NULL);
    }
}

/* ============================
 * PUBLIC BLE API
 * ============================ */

int alpicool_ble_connect(void)
{
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "Already connected");
        return 0;
    }

    ESP_LOGI(TAG, "Starting scan...");

    reconnect_cb(NULL);

    return 0;
}

int alpicool_ble_query_status(void)
{
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE)
        return -1;

    alpicool_cmd_t c = {0};
    memcpy(c.data, ALPICOOL_QUERY, sizeof(ALPICOOL_QUERY));
    c.len = sizeof(ALPICOOL_QUERY);

    xQueueSend(s_cmd_queue, &c, 0);

    return 0;
}

bool alpicool_ble_is_connected(void)
{
    return conn_handle != BLE_HS_CONN_HANDLE_NONE;
}
#include "alpicool.h"
#include "alpicool_ble.h"
#include "alpicool_ui.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "ALPICOOL_SM";

typedef enum {
    ALP_CMD_NONE = 0,
    ALP_CMD_SET_TEMP,
    ALP_CMD_SET_MODE,
    ALP_CMD_SET_POWER
} alpicool_cmd_type_t;

typedef struct {
    alpicool_cmd_type_t type;
    int8_t  temp;
    uint8_t mode;
    uint8_t power;
} alpicool_cmd_t;

static QueueHandle_t s_cmd_queue = NULL;

static alpicool_status_t s_last_status      = {0};
static alpicool_status_t s_before_off       = {0};
static bool              s_before_off_valid = false;

#define ALPICOOL_STATUS_PERIOD_MS 60000

static TickType_t s_next_allowed_status = 0;

/* ============================
 * NVS SAVE / LOAD
 * ============================ */
static void save_status_nvs(const alpicool_status_t *st)
{
    nvs_handle_t h;
    if (nvs_open("alpicool", NVS_READWRITE, &h) != ESP_OK) return;

    nvs_set_i8(h, "set_temp",     st->set_temp);
    nvs_set_i8(h, "real_temp",    st->real_temp);
    nvs_set_u8(h, "mode",         st->mode);
    nvs_set_u8(h, "power",        st->power);
    nvs_set_u8(h, "lock",         st->lock);
    nvs_set_u8(h, "protect",      st->batt_protect);
    nvs_set_u8(h, "battery",      st->battery);

    uint8_t vi = (uint8_t)st->voltage;
    uint8_t vd = (uint8_t)((st->voltage - vi) * 10);

    nvs_set_u8(h, "volt_i", vi);
    nvs_set_u8(h, "volt_d", vd);

    nvs_commit(h);
    nvs_close(h);
}

static bool load_status_nvs(alpicool_status_t *st)
{
    nvs_handle_t h;
    if (nvs_open("alpicool", NVS_READONLY, &h) != ESP_OK)
        return false;

    bool ok = true;

    ok &= nvs_get_i8(h, "set_temp",  &st->set_temp)     == ESP_OK;
    ok &= nvs_get_i8(h, "real_temp", &st->real_temp)    == ESP_OK;
    ok &= nvs_get_u8(h, "mode",      &st->mode)         == ESP_OK;
    ok &= nvs_get_u8(h, "power",     &st->power)        == ESP_OK;
    ok &= nvs_get_u8(h, "lock",      &st->lock)         == ESP_OK;
    ok &= nvs_get_u8(h, "protect",   &st->batt_protect) == ESP_OK;
    ok &= nvs_get_u8(h, "battery",   &st->battery)      == ESP_OK;

    uint8_t vi = 0, vd = 0;
    ok &= nvs_get_u8(h, "volt_i", &vi) == ESP_OK;
    ok &= nvs_get_u8(h, "volt_d", &vd) == ESP_OK;

    st->voltage = vi + (vd / 10.0f);

    nvs_close(h);
    return ok;
}

/* ============================
 * STATUS UPDATE
 * ============================ */
static void update_last_status(const alpicool_status_t *st)
{
    if (!st) return;
    s_last_status = *st;
    save_status_nvs(st);
}

/* ============================
 * PERIODIC STATUS
 * ============================ */
static void session_status(void)
{
    if (!alpicool_ble_is_connected()) {
        ESP_LOGI(TAG, "No BLE connection, starting connect...");
        alpicool_ble_connect();
        return;
    }
}

/* ============================
 * COMMAND SESSION
 * ============================ */
static void session_command(const alpicool_cmd_t *cmd)
{
    if (!cmd) return;

	/* последният известен статус */
	alpicool_status_t st = s_last_status;
	
	/* поискай нов статус за следващия цикъл */
	alpicool_ble_query_status();

    /* SHORT TEMP */
    if (cmd->type == ALP_CMD_SET_TEMP) {
        alpicool_ble_send_temp_short(cmd->temp);
        session_status();
        return;
    }

    /* POWER OFF */
    if (cmd->type == ALP_CMD_SET_POWER && cmd->power == 0) {
        s_before_off       = st;
        s_before_off_valid = true;

        st.power = 0;

        alpicool_ble_send_universal(
            st.set_temp, st.mode, st.power, st.lock, st.batt_protect
        );

        session_status();
        return;
    }

    /* POWER ON */
    if (cmd->type == ALP_CMD_SET_POWER && cmd->power == 1) {
        if (s_before_off_valid)
            st = s_before_off;

        st.power = 1;

        alpicool_ble_send_universal(
            st.set_temp, st.mode, st.power, st.lock, st.batt_protect
        );

        session_status();
        return;
    }

    /* MODE */
    if (cmd->type == ALP_CMD_SET_MODE) {
        st.mode = cmd->mode;

        alpicool_ble_send_universal(
            st.set_temp, st.mode, st.power, st.lock, st.batt_protect
        );

        session_status();
        return;
    }
}

/* ============================
 * TASK LOOP
 * ============================ */
static void alpicool_task(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        alpicool_cmd_t cmd;

        if (xQueueReceive(s_cmd_queue, &cmd, pdMS_TO_TICKS(100))) {

            session_command(&cmd);

            /* BLOCK STATUS FOR 60 SEC */
            s_next_allowed_status = xTaskGetTickCount() + pdMS_TO_TICKS(60000);

            last_wake = xTaskGetTickCount();
        } else {
            TickType_t now = xTaskGetTickCount();

            if (now >= s_next_allowed_status &&
                (now - last_wake) >= pdMS_TO_TICKS(ALPICOOL_STATUS_PERIOD_MS)) {

                session_status();
                last_wake = now;
            }
        }
    }
}

/* ============================
 * INIT
 * ============================ */
void alpicool_init(void)
{
    ESP_LOGI(TAG, "Alpicool state machine init");

    nvs_flash_init();

    if (load_status_nvs(&s_last_status)) {
        ESP_LOGI(TAG, "Loaded status from NVS");
    }

    if (!s_cmd_queue)
        s_cmd_queue = xQueueCreate(8, sizeof(alpicool_cmd_t));

    xTaskCreatePinnedToCore(
        alpicool_task,
        "alpicool_task",
        4096,
        NULL,
        5,
        NULL,
        0
    );
}

void alpicool_get_status(alpicool_status_t *out)
{
    if (out) *out = s_last_status;
}

void alpicool_request_status(void)
{
    session_status();
}

int alpicool_set_temp(int8_t t)
{
    if (t < -20 || t > 20) return -1;

    alpicool_cmd_t cmd = {
        .type = ALP_CMD_SET_TEMP,
        .temp = t
    };

    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE ? 0 : -1;
}

int alpicool_set_mode(uint8_t eco)
{
    if (eco > 1) eco = 1;

    alpicool_cmd_t cmd = {
        .type = ALP_CMD_SET_MODE,
        .mode = eco
    };

    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE ? 0 : -1;
}

int alpicool_set_power(uint8_t on)
{
    if (on > 1) on = 1;

    alpicool_cmd_t cmd = {
        .type = ALP_CMD_SET_POWER,
        .power = on
    };

    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE ? 0 : -1;
}

void alpicool_status_received(const alpicool_status_t *st)
{
    update_last_status(st);
	alpicool_ui_update(st);
}

const alpicool_status_t *alpicool_get_status_ptr(void)
{
    return &s_last_status;
}

bool alpicool_ready_for_sleep(void)
{
    return !alpicool_ble_is_connected();
}

void alpicool_prepare_sleep(void)
{
    if (alpicool_ble_is_connected()) {
        ESP_LOGI(TAG, "Disconnecting fridge before sleep");
        alpicool_ble_disconnect();
    }
}

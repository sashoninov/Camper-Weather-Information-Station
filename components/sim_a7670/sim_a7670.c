/******************************************************************************
 * sim_a7670.c
 *
 * PART 1
 *  - UART Engine
 *  - Modem Mutex
 *  - AT Transaction Layer
 *
 ******************************************************************************/

#include "sim_a7670.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/* -------------------------------------------------------------------------- */
/* CONFIG                                                                     */
/* -------------------------------------------------------------------------- */

#define TAG                     "A7670"

#define MODEM_UART              UART_NUM_3

#define MODEM_TX_PIN            21
#define MODEM_RX_PIN            22

#define MODEM_BAUDRATE          115200

#define MODEM_RX_BUFFER         4096
#define MODEM_TX_BUFFER         4096

#define MODEM_LINE_SIZE         256

#define MODEM_POLL_MS           20

#define MODEM_TIMEOUT_SHORT     2000
#define MODEM_TIMEOUT_NORMAL    5000
#define MODEM_TIMEOUT_HTTP      30000

/* -------------------------------------------------------------------------- */
/* TYPES                                                                      */
/* -------------------------------------------------------------------------- */

typedef enum
{
    MODEM_RES_TIMEOUT = -1,
    MODEM_RES_ERROR   = 0,
    MODEM_RES_OK      = 1

} modem_result_t;

/* -------------------------------------------------------------------------- */
/* GLOBALS                                                                    */
/* -------------------------------------------------------------------------- */

static SemaphoreHandle_t modem_mutex = NULL;

static bool modem_initialized = false;

/* -------------------------------------------------------------------------- */
/* GSM STATUS                                                                 */
/* -------------------------------------------------------------------------- */

gsm_status_t gsm_status = {0};

/* -------------------------------------------------------------------------- */
/* MUTEX                                                                      */
/* -------------------------------------------------------------------------- */

static void modem_lock(void)
{
    xSemaphoreTake(modem_mutex, portMAX_DELAY);
}

static void modem_unlock(void)
{
    xSemaphoreGive(modem_mutex);
}

/* -------------------------------------------------------------------------- */
/* UART                                                                       */
/* -------------------------------------------------------------------------- */

static void uart_flush_all(void)
{
    uart_flush_input(MODEM_UART);

    uint8_t dummy[64];

    while (uart_read_bytes(
               MODEM_UART,
               dummy,
               sizeof(dummy),
               0) > 0)
    {
    }
}

/* -------------------------------------------------------------------------- */

static bool uart_send_raw(const char *text)
{
    if (text == NULL)
        return false;

    ESP_LOGI(TAG, "TX -> %s", text);

    uart_write_bytes(MODEM_UART, text, strlen(text));
    uart_write_bytes(MODEM_UART, "\r\n", 2);

    return true;
}

/* -------------------------------------------------------------------------- */

static int uart_read_byte(uint8_t *ch, uint32_t timeout_ms)
{
    return uart_read_bytes(
        MODEM_UART,
        ch,
        1,
        pdMS_TO_TICKS(timeout_ms));
}

/* -------------------------------------------------------------------------- */

static bool uart_read_line(
    char *line,
    size_t maxlen,
    uint32_t timeout_ms)
{
    if (maxlen == 0)
        return false;

    size_t pos = 0;

    int64_t start = esp_timer_get_time() / 1000;

    while ((esp_timer_get_time() / 1000) - start < timeout_ms)
    {
        uint8_t c;

        if (uart_read_byte(&c, MODEM_POLL_MS) != 1)
            continue;

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            if (pos == 0)
                continue;

            line[pos] = 0;

            ESP_LOGI(TAG, "RX <- %s", line);

            return true;
        }

        if (isprint(c))
        {
            if (pos < maxlen - 1)
            {
                line[pos++] = c;
            }
        }
    }

    return false;
}



/* -------------------------------------------------------------------------- */
/* PUBLIC                                                                     */
/* -------------------------------------------------------------------------- */

bool sim_a7670_init(void)
{
    if (modem_initialized)
        return true;

    const uart_config_t cfg =
    {
        .baud_rate = MODEM_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        MODEM_UART,
        MODEM_RX_BUFFER,
        MODEM_TX_BUFFER,
        0,
        NULL,
        0));

    ESP_ERROR_CHECK(uart_param_config(
        MODEM_UART,
        &cfg));

    ESP_ERROR_CHECK(uart_set_pin(
        MODEM_UART,
        MODEM_TX_PIN,
        MODEM_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));

    modem_mutex = xSemaphoreCreateMutex();

    if (modem_mutex == NULL)
        return false;

    modem_initialized = true;

    ESP_LOGI(TAG, "Driver initialized");

    return true;
}


/* -------------------------------------------------------------------------- */
/* PART 2                                                                     */
/*  AT RESPONSE ENGINE                                                        */
/* -------------------------------------------------------------------------- */

typedef enum
{
    AT_STATUS_NONE = 0,
    AT_STATUS_OK,
    AT_STATUS_ERROR,
    AT_STATUS_TIMEOUT

} at_status_t;

typedef struct
{
    at_status_t status;

    char lines[32][MODEM_LINE_SIZE];

    int count;

} at_response_t;

static at_response_t g_at_response;

/* -------------------------------------------------------------------------- */

static void at_response_clear(at_response_t *r)
{
    memset(r, 0, sizeof(*r));
}

/* -------------------------------------------------------------------------- */

static void at_response_add(at_response_t *r, const char *line)
{
    if (r->count >= 32)
        return;

    snprintf(
        r->lines[r->count],
        MODEM_LINE_SIZE,
        "%s",
        line);

    r->count++;
}

/* -------------------------------------------------------------------------- */

static bool at_is_final_ok(const char *line)
{
    return strcmp(line, "OK") == 0;
}

/* -------------------------------------------------------------------------- */

static bool at_is_final_error(const char *line)
{
    if (!strcmp(line, "ERROR"))
        return true;

    if (!strncmp(line, "+CME ERROR", 10))
        return true;

    if (!strncmp(line, "+CMS ERROR", 10))
        return true;

    return false;
}

/* -------------------------------------------------------------------------- */

static bool at_find_prefix(
    const at_response_t *resp,
    const char *prefix,
    char *out,
    size_t outlen)
{
    for (int i = 0; i < resp->count; i++)
    {
        if (!strncmp(resp->lines[i],
                     prefix,
                     strlen(prefix)))
        {
            strncpy(out,
                    resp->lines[i],
                    outlen - 1);

            out[outlen - 1] = 0;

            return true;
        }
    }

    return false;
}

/* -------------------------------------------------------------------------- */

static at_status_t modem_collect_response(
    at_response_t *resp,
    uint32_t timeout_ms)
{
    char line[MODEM_LINE_SIZE];

    at_response_clear(resp);

    int64_t start =
        esp_timer_get_time() / 1000;

    while ((esp_timer_get_time() / 1000) - start <
           timeout_ms)
    {
        if (!uart_read_line(
                line,
                sizeof(line),
                MODEM_POLL_MS))
        {
            continue;
        }

        at_response_add(resp, line);

        if (at_is_final_ok(line))
        {
            resp->status = AT_STATUS_OK;
            return AT_STATUS_OK;
        }

        if (at_is_final_error(line))
        {
            resp->status = AT_STATUS_ERROR;
            return AT_STATUS_ERROR;
        }
    }

    resp->status = AT_STATUS_TIMEOUT;

    return AT_STATUS_TIMEOUT;
}

/* -------------------------------------------------------------------------- */

static bool modem_execute(
    const char *cmd,
    at_response_t *resp,
    uint32_t timeout_ms)
{
    uart_flush_all();

    if (!uart_send_raw(cmd))
        return false;

    return modem_collect_response(
               resp,
               timeout_ms) == AT_STATUS_OK;
}

/* -------------------------------------------------------------------------- */

static bool modem_expect(
    const char *cmd,
    const char *prefix,
    char *result,
    size_t result_size,
    uint32_t timeout_ms)
{
    if (!modem_execute(
            cmd,
            &g_at_response,
            timeout_ms))
    {
        return false;
    }

    if (prefix == NULL)
    {
        return true;
    }

    return at_find_prefix(
        &g_at_response,
        prefix,
        result,
        result_size);
}

/* -------------------------------------------------------------------------- */
/* TEST HELPERS                                                               */
/* -------------------------------------------------------------------------- */

static bool modem_test(void)
{
    if (!modem_execute(
            "AT",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        ESP_LOGE(TAG, "AT failed");
        return false;
    }

    ESP_LOGI(TAG,
             "AT OK (%d lines)",
             g_at_response.count);

    return true;
}

/* -------------------------------------------------------------------------- */

static bool modem_ping(void)
{
    if (!modem_execute(
            "AT",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        gsm_status.online = false;
        return false;
    }

    gsm_status.online = true;
    return true;
}

/* -------------------------------------------------------------------------- */

static bool modem_echo_off(void)
{
    if (!modem_execute(
            "ATE0",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        ESP_LOGE(TAG, "ATE0 failed");
        return false;
    }

    return true;
}

/* -------------------------------------------------------------------------- */

static bool modem_gnss_power_on(void)
{
    if (!modem_execute(
            "AT+CGNSSPWR=1,1,1",
            &g_at_response,
            MODEM_TIMEOUT_NORMAL))
    {
        ESP_LOGE(TAG,
                 "GNSS power failed");
        return false;
    }

    gsm_status.gnss = true;

    return true;
}

static bool modem_check_gnss(void)
{
    char line[MODEM_LINE_SIZE];

    if (!modem_expect(
            "AT+CGNSSPWR?",
            "+CGNSSPWR:",
            line,
            sizeof(line),
            MODEM_TIMEOUT_SHORT))
    {
        gsm_status.gnss = false;
        return false;
    }

    if (strstr(line, ": 1"))
    {
        gsm_status.gnss = true;
        return true;
    }

    ESP_LOGI(TAG, "GNSS is OFF, enabling...");

    if (!modem_gnss_power_on())
    {
        gsm_status.gnss = false;
        return false;
    }

    gsm_status.gnss = true;
    return true;
}

static bool modem_read_signal(void)
{
    char line[MODEM_LINE_SIZE];

    if (!modem_expect(
            "AT+CSQ",
            "+CSQ:",
            line,
            sizeof(line),
            MODEM_TIMEOUT_SHORT))
    {
        return false;
    }

    int rssi;
    int ber;

    if (sscanf(line, "+CSQ: %d,%d", &rssi, &ber) != 2)
        return false;

    gsm_status.rssi = rssi;

    if (rssi == 99)
    {
        gsm_status.bars = 0;
    }
    else if (rssi >= 25)
    {
        gsm_status.bars = 5;
    }
    else if (rssi >= 20)
    {
        gsm_status.bars = 4;
    }
    else if (rssi >= 15)
    {
        gsm_status.bars = 3;
    }
    else if (rssi >= 10)
    {
        gsm_status.bars = 2;
    }
    else if (rssi >= 5)
    {
        gsm_status.bars = 1;
    }
    else
    {
        gsm_status.bars = 0;
    }

    ESP_LOGI(TAG,
             "RSSI=%d  Bars=%d",
             gsm_status.rssi,
             gsm_status.bars);

    return true;
}

static bool modem_read_registration(void)
{
    char line[MODEM_LINE_SIZE];

    if (!modem_expect(
            "AT+CREG?",
            "+CREG:",
            line,
            sizeof(line),
            MODEM_TIMEOUT_SHORT))
    {
        gsm_status.registered = false;
        gsm_status.ready = false;
        return false;
    }

    bool registered =
        strstr(line, ",1") ||
        strstr(line, ",5");

    gsm_status.registered = registered;
    gsm_status.ready = registered;

    ESP_LOGI(TAG,
             "Registered: %s",
             registered ? "YES" : "NO");

    return true;
}

static bool modem_read_operator(void)
{
    char line[MODEM_LINE_SIZE];

    if (!modem_expect(
            "AT+COPS?",
            "+COPS:",
            line,
            sizeof(line),
            MODEM_TIMEOUT_SHORT))
    {
        gsm_status.operator_name[0] = 0;
        return false;
    }

	char name[32] = {0};
	int rat = -1;
	
	if (sscanf(line,
			"+COPS: %*d,%*d,\"%31[^\"]\",%d",
			name,
			&rat) == 2)
    {
        strncpy(gsm_status.operator_name,
                name,
                sizeof(gsm_status.operator_name) - 1);

        gsm_status.operator_name[
            sizeof(gsm_status.operator_name) - 1] = 0;

		switch (rat)
		{
			case 0:
				strcpy(gsm_status.network, "GSM");
				break;
	
			case 2:
				strcpy(gsm_status.network, "3G");
				break;
	
			case 7:
				strcpy(gsm_status.network, "4G");
				break;
	
			case 9:
				strcpy(gsm_status.network, "NB-IoT");
				break;
	
			default:
				strcpy(gsm_status.network, "N/A");
				break;
		}			
			

		ESP_LOGI(TAG,
				"Operator: %s (%s)",
				gsm_status.operator_name,
				gsm_status.network);

        return true;
    }

    gsm_status.operator_name[0] = 0;

    return false;
}
/* -------------------------------------------------------------------------- */

static bool modem_wait_registration(void)
{
    char line[MODEM_LINE_SIZE];

    for (int retry = 0; retry < 30; retry++)
    {
        if (modem_expect(
                "AT+CREG?",
                "+CREG:",
                line,
                sizeof(line),
                MODEM_TIMEOUT_SHORT))
        {
            if (strstr(line, ",1") ||
                strstr(line, ",5"))
            {
                ESP_LOGI(TAG,
                         "Network registered");

                return true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG,
             "Network registration timeout");

    return false;
}

/* -------------------------------------------------------------------------- */
/* PART 3                                                                     */
/* HTTP ENGINE                                                                */
/* -------------------------------------------------------------------------- */

#define HTTP_MAX_HEADER      128

typedef struct
{
    int method;
    int status;
    int length;

} http_action_t;

/* -------------------------------------------------------------------------- */

static bool http_wait_action(
    http_action_t *action,
    uint32_t timeout_ms)
{
    char line[MODEM_LINE_SIZE];

    int64_t start = esp_timer_get_time() / 1000;

    while ((esp_timer_get_time() / 1000) - start < timeout_ms)
    {
        if (!uart_read_line(
                line,
                sizeof(line),
                MODEM_POLL_MS))
        {
            continue;
        }

        if (strncmp(line, "+HTTPACTION:", 12))
            continue;

        memset(action, 0, sizeof(*action));

        if (strstr(line, "chunk"))
        {
            if (sscanf(
                    line,
                    "+HTTPACTION: %d,%d,chunk",
                    &action->method,
                    &action->status) != 2)
            {
                if (sscanf(
                        line,
                        "+HTTPACTION:%d,%d,chunk",
                        &action->method,
                        &action->status) != 2)
                {
                    ESP_LOGE(TAG,
                             "Invalid HTTPACTION: %s",
                             line);
                    return false;
                }
            }

            action->length = -1;

            ESP_LOGI(TAG,
                     "HTTP status=%d chunked",
                     action->status);

            return true;
        }

        if (sscanf(
                line,
                "+HTTPACTION:%d,%d,%d",
                &action->method,
                &action->status,
                &action->length) != 3)
        {
            if (sscanf(
                    line,
                    "+HTTPACTION: %d,%d,%d",
                    &action->method,
                    &action->status,
                    &action->length) != 3)
            {
                ESP_LOGE(TAG,
                         "Invalid HTTPACTION: %s",
                         line);
                return false;
            }
        }

        ESP_LOGI(TAG,
                 "HTTP status=%d len=%d",
                 action->status,
                 action->length);

        return true;
    }

    ESP_LOGE(TAG, "HTTPACTION timeout");

    return false;
}
/* -------------------------------------------------------------------------- */

static bool http_wait_header(int *payload_size)
{
    char line[MODEM_LINE_SIZE];

    if (payload_size == NULL)
        return false;

    while (1)
    {
        if (!uart_read_line(
                line,
                sizeof(line),
                MODEM_TIMEOUT_HTTP))
        {
            ESP_LOGE(TAG, "HTTPREAD header timeout");
            return false;
        }

        /* Ignore everything until +HTTPREAD */
        if (strncmp(line, "+HTTPREAD:", 10) != 0)
            continue;

        if (sscanf(line, "+HTTPREAD:%d", payload_size) == 1)
        {
            ESP_LOGI(TAG, "HTTPREAD block = %d", *payload_size);
            return true;
        }

        ESP_LOGE(TAG, "Invalid HTTPREAD header: %s", line);
        return false;
    }
}

/* -------------------------------------------------------------------------- */

static bool uart_read_exact(
    uint8_t *buffer,
    size_t size,
    uint32_t timeout_ms)
{
    size_t total = 0;

    int64_t start =
        esp_timer_get_time() / 1000;

    while (total < size)
    {
        if ((esp_timer_get_time() / 1000) - start >
            timeout_ms)
        {
            ESP_LOGE(TAG,
                     "UART timeout (%u/%u)",
                     (unsigned)total,
                     (unsigned)size);

            return false;
        }

        int n = uart_read_bytes(
            MODEM_UART,
            buffer + total,
            size - total,
            pdMS_TO_TICKS(MODEM_POLL_MS));

        if (n > 0)
        {
            total += n;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------- */

static int http_read_payload(char *buffer,
                             size_t buffer_size,
                             int payload_size)
{
    int total = 0;
    int offset = 0;
    int retries = 0;
    char cmd[32];

    (void)payload_size;

    while (1)
    {
        snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=%d,1024", offset);
        uart_send_raw(cmd);

        int block_size;

        if (!http_wait_header(&block_size))
            return -1;

        if (block_size == 0)
        {
            /* Fixed-length download finished */
            if (payload_size > 0 && total >= payload_size)
                break;

            /* Chunked transfer:
               modem may temporarily have no data yet */
            if (++retries >= 5)
                break;

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        retries = 0;

        ESP_LOGI(TAG,
                 "Reading block %d bytes",
                 block_size);

        if (total + block_size >= (int)buffer_size)
        {
            ESP_LOGW(TAG,
                     "HTTP response truncated");

            block_size = buffer_size - total - 1;

            if (block_size <= 0)
                break;
        }

        if (!uart_read_exact(
                (uint8_t *)&buffer[total],
                block_size,
                MODEM_TIMEOUT_HTTP))
        {
            return -1;
        }

        total += block_size;
        offset += block_size;

        buffer[total] = 0;
		

        /* Skip CR/LF after payload */
        uint8_t c;

        uart_read_bytes(MODEM_UART,
                        &c,
                        1,
                        pdMS_TO_TICKS(20));

        if (c == '\r')
        {
            uart_read_bytes(MODEM_UART,
                            &c,
                            1,
                            pdMS_TO_TICKS(20));
        }
    }

    buffer[total] = 0;

    ESP_LOGI(TAG,
             "HTTP payload = %d bytes",
             total);

    return total;
}

/* -------------------------------------------------------------------------- */

static bool http_term(void)
{
    return modem_execute(
        "AT+HTTPTERM",
        &g_at_response,
        MODEM_TIMEOUT_SHORT);
}

/* -------------------------------------------------------------------------- */

int sim_a7670_http_get(
    const char *url,
    char *buffer,
    size_t buffer_size)
{
    modem_lock();
	
	if (!gsm_status.ready)
    {
        modem_unlock();

        ESP_LOGW(TAG, "HTTP skipped: modem not registered");

        return -1;
    }

    http_action_t action;
    int payload_size = 0;
    char cmd[1024];

    uart_flush_all();

    /* ------------------------------------------------------------ */

    if (!modem_execute(
            "AT+HTTPINIT",
            &g_at_response,
            MODEM_TIMEOUT_NORMAL))
    {
        goto fail;
    }

    /* ------------------------------------------------------------ */

    snprintf(
        cmd,
        sizeof(cmd),
        "AT+HTTPPARA=\"URL\",\"%s\"",
        url);

    if (!modem_execute(
            cmd,
            &g_at_response,
            MODEM_TIMEOUT_NORMAL))
    {
        goto fail;
    }

    /* ------------------------------------------------------------ */

    if (!modem_execute(
            "AT+HTTPACTION=0",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        goto fail;
    }

    /* ------------------------------------------------------------ */

    if (!http_wait_action(
            &action,
            MODEM_TIMEOUT_HTTP))
    {
        goto fail;
    }

    if (action.status != 200)
    {
        ESP_LOGE(TAG,
                 "HTTP status=%d",
                 action.status);

        goto fail;
    }

	if (action.length == 0)
	{
		ESP_LOGE(TAG,
				"Empty response");
	
		goto fail;
	}

    /* ------------------------------------------------------------ */



    /* ------------------------------------------------------------ */

	payload_size = http_read_payload(
						buffer,
						buffer_size,
						action.length);
	
	if (payload_size < 0)
	{
		goto fail;
	}
    /* ------------------------------------------------------------ */

    modem_collect_response(
        &g_at_response,
        MODEM_TIMEOUT_SHORT);

    http_term();

    modem_unlock();

    ESP_LOGI(TAG, "HTTP GET OK");

    return payload_size;

fail:

    http_term();

    modem_unlock();

    ESP_LOGE(TAG, "HTTP GET FAILED");

    return -1;
}

/* -------------------------------------------------------------------------- */

static void gsm_monitor_task(void *arg)
{
    (void)arg;

    while (1)
    {
		modem_lock();
		
		bool alive = modem_ping();
		
		if (alive)
		{
			modem_read_registration();
			modem_read_signal();
			modem_read_operator();
		}
		
		modem_unlock();
		
		if (!alive)
		{
			ESP_LOGW(TAG, "Modem offline");
		
			gsm_status.ready = false;
		
			vTaskDelay(pdMS_TO_TICKS(5000));
			continue;
		}
		
		vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* -------------------------------------------------------------------------- */
/* PART 4                                                                     */
/* PUBLIC API                                                                 */
/* -------------------------------------------------------------------------- */



/* -------------------------------------------------------------------------- */

bool sim_a7670_get_time(struct tm *utc)
{
    if (utc == NULL)
        return false;

    memset(utc, 0, sizeof(*utc));

    /*
     * Временна реализация.
     * По-късно ще се използва:
     *
     * AT+CCLK?
     *
     * или GPS UTC.
     */

    utc->tm_year = 2026 - 1900;
    utc->tm_mon  = 6;
    utc->tm_mday = 18;

    utc->tm_hour = 12;
    utc->tm_min  = 0;
    utc->tm_sec  = 0;

    return true;
}

/* -------------------------------------------------------------------------- */

bool sim_a7670_start(void)
{
    if (!modem_initialized)
        return false;

    modem_lock();

	if (!modem_test())
		goto fail;
	
	if (!modem_echo_off())
		goto fail;
	
	/* Request long operator name instead of numeric MCC/MNC */
	if (!modem_execute(
			"AT+COPS=3,0",
			&g_at_response,
			MODEM_TIMEOUT_SHORT))
	{
		ESP_LOGW(TAG, "Failed to set operator format");
	}
	
	/*
	* Единствената конфигурация,
	* която правим при стартиране.
	*/
	
	if (!modem_check_gnss())
		goto fail;
	
	if (!modem_wait_registration())
	{
		ESP_LOGE(TAG, "Network registration timeout");
		goto fail;
	}
	
	gsm_status.registered = true;
	
	modem_read_operator();
	modem_read_signal();
	

    modem_unlock();

    ESP_LOGI(TAG, "SIM A7670 READY");

    return true;

fail:

    modem_unlock();

    gsm_status.ready = false;
    gsm_status.gnss = false;

    ESP_LOGE(TAG, "SIM A7670 START FAILED");

    return false;
}

/* -------------------------------------------------------------------------- */

static void modem_dump_response(
    const at_response_t *resp)
{
    ESP_LOGI(TAG,
             "----- RESPONSE (%d lines) -----",
             resp->count);

    for (int i = 0; i < resp->count; i++)
    {
        ESP_LOGI(TAG,
                 "[%02d] %s",
                 i,
                 resp->lines[i]);
    }

    ESP_LOGI(TAG,
             "-------------------------------");
}

/* -------------------------------------------------------------------------- */

bool sim_a7670_self_test(void)
{
    modem_lock();

    ESP_LOGI(TAG, "Self test...");

    if (!modem_execute(
            "AT",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        modem_unlock();
        return false;
    }

    modem_dump_response(&g_at_response);

    if (!modem_execute(
            "ATI",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        modem_unlock();
        return false;
    }

    modem_dump_response(&g_at_response);

    if (!modem_execute(
            "AT+CPIN?",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        modem_unlock();
        return false;
    }

    modem_dump_response(&g_at_response);

    if (!modem_execute(
            "AT+CSQ",
            &g_at_response,
            MODEM_TIMEOUT_SHORT))
    {
        modem_unlock();
        return false;
    }

    modem_dump_response(&g_at_response);

    modem_unlock();

    ESP_LOGI(TAG, "Self test OK");

    return true;
}

void sim_a7670_monitor_start(void)
{
    xTaskCreate(
        gsm_monitor_task,
        "gsm_monitor",
        4096,
        NULL,
        5,
        NULL);
}
/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */
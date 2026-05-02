#include "victron_task.h"
#include "victron_driver.h"
#include "app_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static void parse(char *line)
{
    char key[16], val[16];

    if (sscanf(line, "%15s\t%15s", key, val) != 2)
        return;

    if (strcmp(key, "V") == 0)
        app_state.battery_voltage = atof(val) / 1000.0f;

    else if (strcmp(key, "I") == 0)
        app_state.battery_current = atof(val) / 1000.0f;

    else if (strcmp(key, "VPV") == 0)
        app_state.solar_voltage = atof(val) / 1000.0f;

    else if (strcmp(key, "PPV") == 0)
        app_state.solar_power = atoi(val);

    else if (strcmp(key, "CS") == 0)
        app_state.charge_state = atoi(val);

    app_state.mppt_valid = true;
}

void victron_task(void *arg)
{
    victron_init();

    char line[64];

    while (1)
    {
        if (victron_read_line(line, sizeof(line)))
        {
            //printf("RAW: %s\n", line);
           
            parse(line);

           // printf("VE: %s\n", line);
        }
    }
}
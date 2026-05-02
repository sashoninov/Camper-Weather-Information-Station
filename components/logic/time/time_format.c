#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "time_bg.h"
#include "time_format.h"
#include "app_state.h"

void format_time_only(struct tm *timeinfo, bool blink)
{
    snprintf(app_state.time_str, sizeof(app_state.time_str),
             "%02d:%02d",
             timeinfo->tm_hour,
             timeinfo->tm_min);
}

void format_date(struct tm *timeinfo)
{
    snprintf(app_state.date_str, sizeof(app_state.date_str),
             "%d %s",
             timeinfo->tm_mday,
             months_bg[timeinfo->tm_mon]);
}

void format_weekday(struct tm *timeinfo)
{
    snprintf(app_state.weekday_str, sizeof(app_state.weekday_str),
             "%s",
             weekdays_bg[timeinfo->tm_wday]);
}
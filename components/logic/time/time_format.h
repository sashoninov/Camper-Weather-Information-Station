#pragma once
#include <time.h>
#include <stdbool.h>

void format_time_only(struct tm *timeinfo, bool blink);
void format_date(struct tm *timeinfo);
void format_weekday(struct tm *timeinfo);
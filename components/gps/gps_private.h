#pragma once

#include "gps.h"

/* Internal function used only by the NMEA parser */
bool gps_update_data(const gps_data_t *data);

#include "app_state.h"
#include <stdbool.h>

bool weather_get_coordinates(double *lat, double *lon)
{
    bool valid = false;

    app_state_lock();

    if (app_state.gps.valid)
    {
        *lat = app_state.gps.lat;
        *lon = app_state.gps.lon;
        valid = true;
    }
    else if (app_state.location.lat != 0 && app_state.location.lon != 0)
    {
        *lat = app_state.location.lat;
        *lon = app_state.location.lon;
        valid = true;
    }

    app_state_unlock();

    return valid;
}


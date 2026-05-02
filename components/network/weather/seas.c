#include "seas.h"
#include <math.h>

typedef struct {
    const char *name;
    float lat;
    float lon;
} sea_t;

// ⭐ Разширен списък с всички морета от картата
static sea_t seas[] = {
    {"Black Sea",        43.0f, 28.0f},   // Черно море
    {"Aegean Sea",       39.0f, 25.0f},   // Егейско море
    {"Ionian Sea",       38.5f, 20.5f},   // Йонийско море
    {"Adriatic Sea",     43.5f, 16.0f},   // Адриатическо море
    {"Mediterranean Sea",36.0f, 18.0f},   // Средиземно море (централно)
    {"Tyrrhenian Sea",   40.5f, 13.0f},   // Тиренско море
    {"Ligurian Sea",     43.5f, 9.0f},    // Лигурско море
    {"Marmara Sea",      40.7f, 28.0f},   // Мраморно море
};

static float distance(float lat1, float lon1, float lat2, float lon2)
{
    float dlat = lat1 - lat2;
    float dlon = lon1 - lon2;
    return sqrtf(dlat*dlat + dlon*dlon);
}

int find_nearest_sea(float lat, float lon, float *out_lat, float *out_lon)
{
    float min_dist = 999.0f;
    int index = -1;

    int count = sizeof(seas) / sizeof(sea_t);

    for (int i = 0; i < count; i++) {
        float d = distance(lat, lon, seas[i].lat, seas[i].lon);

        if (d < min_dist) {
            min_dist = d;
            index = i;
        }
    }

    // ⭐ Ако си над 2° (~110 км) → твърде далеч от море
    if (min_dist > 2.0f)
        return -1;

    *out_lat = seas[index].lat;
    *out_lon = seas[index].lon;

    return index;
}

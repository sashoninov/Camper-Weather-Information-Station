#include "seas.h"
#include <stdbool.h>
#include <math.h>

#define MAX_SEA_DISTANCE_KM 60.0f

typedef struct
{
    float lat;
    float lon;
} sea_point_t;

/*
 * Coast reference points.
 *
 * These points are used ONLY to determine whether the current GPS
 * position is close enough to a coastline to request data from the
 * Open-Meteo Marine API.
 *
 * Add new points every 20-30 km along the coastline.
 */

static const sea_point_t sea_points[] =
{
    // =====================================================
    // Bulgaria - Black Sea
    // =====================================================
    {43.7420f, 28.5650f}, // Durankulak
    {43.5700f, 28.6100f}, // Shabla
    {43.3650f, 28.4700f}, // Kavarna
    {43.2950f, 28.0550f}, // Balchik
    {43.2050f, 27.9300f}, // Golden Sands
    {43.1850f, 27.9150f}, // Varna
    {43.0400f, 27.8900f}, // Kamchia
    {42.8800f, 27.8900f}, // Byala
    {42.8200f, 27.8800f}, // Obzor
    {42.6600f, 27.7300f}, // Sunny Beach
    {42.6600f, 27.7300f}, // Nessebar
    {42.5600f, 27.6400f}, // Pomorie
    {42.5000f, 27.4700f}, // Burgas
    {42.4200f, 27.7000f}, // Chernomorets
    {42.4200f, 27.7000f}, // Sozopol
    {42.2400f, 27.7800f}, // Primorsko
    {42.2300f, 27.8400f}, // Kiten
    {42.2000f, 27.8100f}, // Lozenets
    {42.1700f, 27.8500f}, // Tsarevo
    {42.1000f, 27.9400f}, // Ahtopol
    {42.0600f, 27.9800f}, // Sinemorets
    {41.9830f, 28.0300f}, // Rezovo

    // ---------- Turkey - Black Sea ----------
    {41.9900f, 28.0300f}, // Igneada
    {41.7500f, 32.3900f}, // Amasra
    {41.4300f, 31.7800f}, // Zonguldak
    {41.2900f, 31.4200f}, // Eregli
    {41.1700f, 29.0900f}, // Istanbul North
    {41.0800f, 28.9800f}, // Bosphorus

    // ---------- Sea of Marmara ----------
    {40.9800f, 27.5100f}, // Tekirdag
    {40.4600f, 27.9700f}, // Bandirma
    {40.3500f, 28.8800f}, // Mudanya
    {40.4300f, 29.1600f}, // Gemlik
    {40.7600f, 29.9400f}, // Izmit

    // ---------- Dardanelles ----------
    {40.1500f, 26.4100f}, // Canakkale

    // ---------- North Aegean ----------
    {40.8500f, 25.8700f}, // Alexandroupoli
    {40.9400f, 24.4100f}, // Kavala
    {40.7100f, 24.7300f}, // Keramoti
    {40.7700f, 24.7100f}, // Thasos
    {40.3000f, 23.9000f}, // Mount Athos

    // ---------- Halkidiki ----------
    {40.1000f, 23.9800f}, // Ouranoupoli
    {40.0200f, 23.5600f}, // Vourvourou
    {39.9800f, 23.6100f}, // Sarti
    {39.9900f, 23.3900f}, // Neos Marmaras
    {39.9700f, 23.3700f}, // Porto Carras
    {40.0200f, 23.2000f}, // Nikiti
    {39.9880f, 23.9010f}, // Toroni
	
	// ---------- Olympiada / Stavros ----------
	{40.5907f, 23.7811f}, // Olympiada
	{40.6670f, 23.7000f}, // Stratoni
	{40.6640f, 23.6980f}, // Nea Roda
	{40.6700f, 23.7050f}, // Ierissos
	{40.7050f, 23.7200f}, // Trypiti
	{40.5900f, 23.7950f}, // Ancient Stagira
	{40.6710f, 23.7070f}, // Komitsa Beach
	{40.7400f, 23.8800f}, // Ammouliani Ferry
	{40.6700f, 23.8600f}, // Ammouliani
	{40.7600f, 23.9400f}, // Drenia Islands
	{40.6700f, 23.7060f}, // Nea Roda Beach
	{40.6690f, 23.7280f}, // Xiropotamos
	{40.7400f, 23.7000f}, // Tripiti Port
	
	{40.7860f, 23.8940f}, // Ouranoupoli Port
	{40.3900f, 23.9200f}, // Asprovalta
	{40.7500f, 23.6700f}, // Pyrgadikia
	{39.9460f, 23.5850f}, // Porto Koufo
	{39.9980f, 23.9150f}, // Tristinika
	{39.9900f, 23.9750f}, // Destenika	
	
    // ---------- Thermaic Gulf ----------
    {40.2700f, 22.6000f}, // Paralia Katerini
    {40.6300f, 22.9400f}, // Thessaloniki
    {40.5100f, 22.8300f}, // Agia Triada

    // ---------- Central Greece ----------
    {39.3600f, 22.9400f}, // Volos
    {38.9000f, 22.4300f}, // Kamena Vourla

    // ---------- Evia ----------
    {38.4600f, 23.6000f}, // Chalkida
    {38.3900f, 24.0500f}, // Eretria
    {38.0200f, 24.4200f}, // Karystos

    // ---------- Attica ----------
    {38.0200f, 23.7300f}, // Athens
    {37.8900f, 24.0100f}, // Rafina
    {37.6500f, 24.0200f}, // Sounion

    // ---------- Peloponnese ----------
    {37.9400f, 22.9300f}, // Corinth
    {37.6400f, 22.7300f}, // Nafplio
    {37.0300f, 22.1100f}, // Kalamata

    // ---------- Ionian ----------
    {39.6200f, 19.9200f}, // Corfu
    {38.8300f, 20.7100f}, // Lefkada
    {38.3700f, 20.7200f}, // Kefalonia
    {37.7900f, 20.9000f}, // Zakynthos

    // ---------- Crete ----------
    {35.5200f, 24.0200f}, // Chania
    {35.3400f, 25.1400f}, // Heraklion
    {35.2000f, 26.1000f}, // Agios Nikolaos
	
	// Croatia
	{45.0800f, 13.6380f}, // Rovinj
	{43.5081f, 16.4402f}, // Split
	
	// Montenegro
	{42.4304f, 18.6961f}, // Budva
	{42.2864f, 18.8400f}, // Ulcinj
	
	// Albania
	{41.3133f, 19.4562f}, // Durrës
	{39.8756f, 19.9997f}, // Sarandë

    // ---------- Turkey - Aegean ----------
    {39.0800f, 26.8900f}, // Ayvalik
    {38.4300f, 27.1400f}, // Izmir
    {38.3200f, 26.3000f}, // Cesme
    {37.8600f, 27.2600f}, // Kusadasi
    {37.0400f, 27.4300f}, // Bodrum

    // ---------- Turkey - Mediterranean ----------
    {36.8900f, 30.7100f}, // Antalya
    {36.6000f, 30.5600f}, // Kemer
    {36.5500f, 31.9900f}, // Alanya
    {36.8000f, 34.6300f}, // Mersin
    {36.5800f, 36.1700f}, // Iskenderun
};

static float distance_km(float lat1, float lon1,
                         float lat2, float lon2)
{
    const float R = 6371.0f;

    float dlat = (lat2 - lat1) * (float)M_PI / 180.0f;
    float dlon = (lon2 - lon1) * (float)M_PI / 180.0f;

    float a =
        sinf(dlat * 0.5f) * sinf(dlat * 0.5f) +
        cosf(lat1 * (float)M_PI / 180.0f) *
        cosf(lat2 * (float)M_PI / 180.0f) *
        sinf(dlon * 0.5f) * sinf(dlon * 0.5f);

    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));

    return R * c;
}

bool is_near_sea(float lat, float lon)
{
    const int count = sizeof(sea_points) / sizeof(sea_points[0]);

    for (int i = 0; i < count; i++) {

        float d = distance_km(lat,
                              lon,
                              sea_points[i].lat,
                              sea_points[i].lon);

        if (d <= MAX_SEA_DISTANCE_KM)
            return true;
    }

    return false;
}
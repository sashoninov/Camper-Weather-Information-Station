camper-weather-v2 — Project Overview
====================================

<p align="center">
  <img src="assets/IMG_1.jpg" width="700">
</p>

camper-weather-v2 is an embedded software project for the ESP32‑P4, designed as an autonomous environment, energy and navigation station for camper and RV applications.  
The system integrates multiple hardware modules and provides a stable, modular and power‑efficient platform built on ESP-IDF and LVGL.

Main Features
-------------

- GPS positioning and automatic time synchronization (GPS / Internet)
- Wi‑Fi management and weather forecast retrieval (Open‑Meteo API)
- The weather forecast implementation is largely based on the project by **Harald Kreuzer**:  
  https://www.haraldkreuzer.net/en/news/esp32-weather-station-20-radio-sensors-open-meteo-api-ips-display-mmwave-radar-fine-dust-sensor-and-much-more
- Environmental sensors: temperature, humidity, pressure, ambient light
- Automatic display brightness control
- **Victron SmartSolar integration via UART** (voltage, current, charging state)
- Monitoring of **fresh water** and **grey water** tank levels
- **WC cassette full** detection signal
- **Electronic leveling** (tilt measurement for camper leveling)
- **Fridge temperature** monitoring
- **CO₂ level** monitoring for interior air quality
- LVGL-based UI optimized for the Waveshare 10.1" DSI display
- Multithreaded architecture with dedicated FreeRTOS tasks (GPS, Wi‑Fi, Weather, UI, Time Sync, Sensors, etc.)

Architecture
------------

The project is organized into clean and maintainable modules:

- `components/core` – core system utilities  
- `components/network` – Wi‑Fi, weather API, communication  
- `components/drivers` – display, GPS, sensors, UART (Victron)  
- `components/tasks` – individual FreeRTOS tasks  
- `components/ui` – LVGL screens and UI logic  
- `main` – system initialization and startup  

Technologies
------------

- ESP-IDF  
- LVGL  
- C / C++  
- FreeRTOS  
- I2C / UART / Wi‑Fi / GNSS  

Development
-----------

The entire software codebase was developed with the assistance of **ChatGPT** and **GitHub Copilot**, using iterative refinement, AI‑assisted design and continuous improvement.

Purpose
-------

A reliable and autonomous system for displaying weather, energy data, tank levels and interior environmental conditions — ideal for campers, RVs and off‑grid applications.


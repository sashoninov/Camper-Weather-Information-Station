Camper Weather & Information Station

<p align="center">
  <img src="assets/IMG_1.jpg" width="700">
</p>

A high‑performance, fully autonomous environmental, navigation, and energy monitoring system for campers, RVs, and off‑grid setups — powered by ESP32‑P4.

🚀 Overview
This project integrates weather forecasting, GNSS positioning, energy monitoring, environmental sensing, audio notifications, SD card audio storage, and a modern LVGL‑based UI into a single embedded system.
Designed for reliability, modularity, and long‑term off‑grid operation.

🌍 Core Features
Geolocation & Timekeeping
GNSS positioning (NMEA / u‑blox compatible)

Automatic time synchronization via GPS

Local time maintained by DS3231 RTC

Automatic timezone switching based on GPS coordinates

No NTP required

Weather & Forecasting
Live weather data from Open‑Meteo API

Temperature, humidity, pressure

UV index, cloud cover, precipitation

Weather icons and graphical forecast

Automatic periodic updates

Energy Monitoring
Integration with Victron SmartSolar (UART)

Battery voltage, current, charge state

Solar charging status

Real‑time telemetry

Environmental Sensors
Fresh water tank level

Grey water tank level

WC cassette full indicator

Fridge temperature

CO₂ monitoring

Ambient light sensor (ALS)

Automatic display dimming

User Interface
LVGL‑based UI optimized for Waveshare 10.1" DSI display

Smooth animations and transitions

Multi‑screen layout

Touch control

Day/Night themes

🔊 Audio Subsystem
A fully modular, event‑driven audio architecture designed for reliability and flexibility.

🎵 Audio Manager
Non‑blocking audio playback

Event‑based sound triggering

Priority queue for overlapping events

Automatic PA amplifier control

Fade‑in / Fade‑out transitions

Playback completion callbacks

Hourly chime with correct 24‑hour mapping

Sound suppression when display dimming is active

📁 SD Audio File Manager
WAV file loading from SD card

Header caching for fast access

Directory‑based sound organization

Automatic fallback if SD is missing

Support for custom sound packs

🔔 Audio Events
GPS fix

Wi‑Fi connected

Error / warning

Touch feedback

Alarm

Hourly chime (00–23 → 24‑hour sound set)

Dimming‑mode sound blocking

💾 SD Card Support
The system uses SDMMC for high‑speed SD card access.

Features:
FATFS filesystem

High‑speed streaming of WAV audio

Automatic directory scanning

Logging and error reporting

Hot‑swap detection (optional)

Recommended SD structure:
Код
/sdcard/audio/
    system/
    ui/
    alerts/
    hours/
🧩 Project Architecture
Код
components/
    core/        – system utilities
    network/     – Wi-Fi, HTTP, Open-Meteo
    drivers/     – display, GPS, RTC, sensors, Victron
    audio/       – Audio Manager + SD Audio File Manager
    tasks/       – FreeRTOS tasks
    ui/          – LVGL screens and logic
main/
    app_main.c   – system initialization
🧵 FreeRTOS Task Model
GPS Task

Weather Task

Wi‑Fi Task

UI Task

Time Sync Task

Sensor Task

Audio Task

SD Card Task

Each task is isolated and communicates via event queues.

🛠 Technologies Used
ESP‑IDF

FreeRTOS

LVGL

FATFS

I2C / UART / Wi‑Fi

C / C++

🔧 Build Instructions
Код
idf.py set-target esp32p4
idf.py build
idf.py flash
idf.py monitor
🎯 Project Goal
To create a robust, modular, and visually rich embedded system that provides:

real‑time weather

navigation

energy monitoring

environmental sensing

audio alerts

intuitive UI

All optimized for camper and off‑grid environments.

👤 Development Notes
This project is developed using:

ChatGP

GitHub Copilot

AI‑assisted architecture refinement

Modular component‑based design

# Camper Information & Monitoring System

<p align="center">
  <img src="assets/Image .png" width="700">
</p>

## User Interface

The Camper Information & Monitoring System has been designed around a simple and intuitive user interface that minimizes navigation while providing immediate access to all important information.

Unlike many embedded applications that rely on numerous menus and screens, this project uses only **two primary screens**:

- **Main Dashboard** – the central screen containing all monitoring and control functions.
- **Settings** – configuration of the entire system.

This design philosophy allows the user to access weather information, vehicle status and all camper subsystems with minimal interaction, making the system easier and safer to use during everyday travel.

---

## Display Support

The graphical interface was originally designed for the **Waveshare 10.1-inch DSI Capacitive Touch Display** with a native resolution of **1280 × 800**.

The available screen area allows the complete dashboard to be displayed without overcrowding the interface. Weather information, GPS, energy monitoring, water tank levels, refrigerator control, camper leveling and system status are presented simultaneously in clearly separated sections.

The project is currently developed and tested on the **Waveshare 5-inch DSI Capacitive Touch Display** with a resolution of **1280 × 720**.

Because the 5-inch display provides 80 fewer vertical pixels than the original design, some pages require a short vertical touch scroll to access the lower part of the dashboard. This affects only the visible area—the complete functionality of the application remains available without modification.

Display support currently includes:

| Display | Resolution | Status |
|---------|------------|--------|
| Waveshare 10.1" DSI Capacitive Touch Display | 1280 × 800 | Original user interface design |
| Waveshare 5" DSI Capacitive Touch Display | 1280 × 720 | Current development and testing platform |

---

## Main Dashboard

The Main Dashboard integrates all major camper subsystems into a single screen.

Displayed information includes:

- Current weather conditions
- Multi-day weather forecast
- GPS position and navigation data
- Date and time
- Victron SmartSolar MPPT monitoring (VE.Direct UART)
- Fresh water tank level
- Grey water tank level
- Camper leveling assistant
- Alpicool refrigerator monitoring and control
- LTE communication status
- Wi-Fi connection status
- System notifications and alarms

The dashboard has been carefully arranged to maximize information density while maintaining readability on the target 10.1-inch display.

---

## Camper Leveling Assistant

The project includes a precision leveling assistant based on the **MPU6050** 6-axis inertial measurement unit.

Using accelerometer and gyroscope data, the system continuously measures the camper's pitch and roll and presents the information graphically on the display.

This enables accurate vehicle leveling before parking, improving comfort and ensuring proper operation of equipment such as the refrigerator.

Features include:

- Real-time pitch and roll measurement
- High-precision graphical leveling indicator
- Fast sensor updates
- Continuous visual feedback
- Integrated directly into the main dashboard


# Hardware Overview

The Camper Information & Monitoring System is built around the ESP32-P4 platform and combines multiple sensors and communication interfaces to provide monitoring and control of the camper's essential systems.

## Main Controller

The project supports both Waveshare ESP32-P4 development boards:

- ESP32-P4-Module High-performance Development Board  https://www.waveshare.com/esp32-p4-module-dev-kit.htm?srsltid=AfmBOooXO1gcZ95H76G10vcrTli3m9XoD4qQeatHZypbfZcwjPw9f9YV
- ESP32-P4-WIFI6-DEV-KIT

Both boards are based on the **ESP32-P4** application processor and an **ESP32-C6** wireless coprocessor, providing:

- Wi-Fi 6
- Bluetooth 5 / BLE
- High-performance graphics support
- Multiple UART, I²C and SPI interfaces
- MIPI-DSI display interface
- Capacitive touch support



---

## Display

The graphical interface was originally designed for:

**Waveshare 10.1-inch DSI Capacitive Touch Display**

Resolution:

- 1280 × 800
- IPS panel
- Capacitive multi-touch



During development, the project is primarily tested on:

**Waveshare 5-inch DSI Capacitive Touch Display**

Resolution:

- 1280 × 720
- Capacitive multi-touch



---

## Sensors

### DS3231 Real-Time Clock

Provides an accurate real-time clock and maintains date and time while the system is powered off.

### BME680

Monitors the environmental conditions inside the camper:

- Temperature
- Humidity
- Atmospheric pressure
- Indoor Air Quality (IAQ)

### BH1750

Ambient light sensor used for automatic display brightness adjustment.

### DS18B20

Two waterproof digital temperature sensors are used:

- Outside ambient temperature
- Refrigerator compartment temperature

### INA3221

Three-channel voltage monitor used for:

- Starter battery voltage
- Fresh water tank level
- Grey water tank level

### MPU6050

Six-axis IMU used by the Camper Leveling Assistant to measure vehicle pitch and roll for precise leveling.

---

## Communication Modules

### Victron SmartSolar MPPT

Connected via UART (VE.Direct).

Provides real-time monitoring of:

- House battery status
- Solar charging information
- Battery voltage
- Battery current
- Solar panel voltage
- Power generation
- System load

### Alpicool Portable Refrigerator

The system integrates directly with compatible **Alpicool portable refrigerators** over **Bluetooth Low Energy (BLE)**.

The built-in BLE client provides both real-time monitoring and remote control of the refrigerator.

Available features include:

- Refrigerator connection status
- Current internal temperature
- Target temperature
- Cooling mode
- Battery protection level
- Compressor status
- Power on/off control
- Temperature adjustment
- Operating mode selection

The refrigerator is fully integrated into the Main Dashboard, allowing monitoring and control without using the manufacturer's mobile application.

### SIMCom A7670E LTE Modem

Connected through dedicated UART interfaces.

Provides:

- 4G LTE Internet connectivity
- HTTP requests for weather services
- GPS receiver
- Location information

Product page:

https://www.and-global.com/list_23/568.html



# AirStation — Real-Time IoT Monitor

## Live Dashboard
https://dhruvjain1583-dev.github.io/airstation-iot-dashboard

## Wokwi Simulation

https://wokwi.com/projects/459033289778557953

## What It Does
Real-time gas, smoke, temperature, humidity and light
monitoring with automatic alerts and live web dashboard.

## Full Pipeline
ESP32 → WiFi → MQTT → HiveMQ Cloud → WebSocket → Dashboard

## Tech Stack
- Hardware: ESP32, MQ-2, DHT22, LDR, SSD1306 OLED
- Protocol: MQTT (HiveMQ Broker)
- Dashboard: HTML + CSS + JavaScript + Chart.js
- Simulation: Wokwi IoT Simulator
- Hosting: GitHub Pages

## Features
- Live sensor readings updated every 5 seconds
- Automatic gas/smoke threshold alert system
- Real-time historical charts
- Zero backend — pure frontend WebSocket connection
- Fully cloud-connected IoT pipeline

## Circuit Connections
| Component | ESP32 Pin |
|-----------|-----------|
| DHT22 DATA | GPIO 4 |
| MQ2 AO | GPIO 34 |
| MQ2 DO | GPIO 35 |
| LDR AO | GPIO 33 |
| LDR DO | GPIO 32 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| LED | GPIO 2 |

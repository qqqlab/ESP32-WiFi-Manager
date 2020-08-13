# ESP32-WiFi-Manager

A WiFi manager specifically for ESP32. I tried many of the other options available, but got none of them to do exactly what I wanted. Also writing my own version proved tricky because of random crashes see for example [here](https://github.com/espressif/arduino-esp32/issues/2025).

## Features
- one page setup, dhcp only
- automatic fallback to setup when wifi connection drops
- automatic reestablish connection when in setup mode

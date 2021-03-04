# ESP32-WiFi-Manager

A WiFi manager (Captive Portal) specifically for ESP32 in unreliable network environments. 

The following timeouts allow recovery from dropped/reappearing networks, whilst allowing the user to enter new wifi credentials if needed:

- _connectingTimeout_ wait for initial wifi connection before starting captive portal (default 20 seconds)
- _lostConnectionTimeout_ wait for lost wifi to reconnect before starting captive portal (default 60 seconds)
- _portalTimeout_ time to keep captive portal alive before rebooting (default 300 seconds)

Status cycle:

1) _CONNECTING_ On startup connect to wifi, if not successful within _connectingTimeout_ the captive portal is started.

2) _CONNECTED_ If connection is lost, try to reconnect. If not successful within _lostConnectionTimeout_ the captive portal is started.

3) _PORTAL_ After entering wifi credentials in the captive portal, or if no connection is made to the captive portal within _portalTimeout_, reboot and try to connect to the wifi network.

I tried many of the other options available, but got none of them to do exactly what I wanted. Also writing my own version proved tricky because of random crashes see for example [here](https://github.com/espressif/arduino-esp32/issues/2025).

## Features
- small code size
- captive portal with single page to set wifi credentials
- automatic fallback to captive portal when wifi connection drops
- automatic reestablish connection

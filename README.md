# esp-ener2mqtt
Energenie to MQTT for ESP8266 devices

Designed to work with the RFM69W 433Mhz HopeRF Wireless Transceiver and an ESP8266 MCU. Should work with either a bare board or the ENER314-RT module. If using a bare board connect an antenna (17.2mm wire should work) to the ANA point.

# Supported devices
Recieves messages from the Energenie devices that use the OpenThings protocol. This is useful for integrating door sensors and power monitors into home automation systems.

Transmit function not yet implemented.

https://energenie4u.co.uk/

# Conections from RFM69W to ESP8266

RFM69W | NodeMCU | ESP8266
--- | --- | --- |
MSS | D8 | CS
MOSI | D7 | MOSI
MISO | D6 | MISO
SCK | D5 | SCLK
GND | GND | GND
3.3 | 3.3 | GND
RESET | D1 | GPIO5

# Usage
Compile with Arduino 1.8.x. Flash to MCU with Tazmotizer. On the first power-up it will create a hotspot that allows configuation details to be entered.
Default MQTT topic is ener/type/device-id/data

# Authors
* Kristian Shaw - ESP8266 implementation with MQTT support
* David Whale (whaleygeek) - Original Raspberry Pi C implementation with ENER314-RT module
* Philip Grainger (Achronite) - Improved Raspberry Pi implemenation with NodeRed and MQTT functionality

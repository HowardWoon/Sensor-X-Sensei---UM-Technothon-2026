# Hardware Schematic

This document outlines the pin connections for the Sensor X Sensei prototype.

## ESP32 DOIT DEVKIT V1

| Component | Pin Function | ESP32 Pin |
| :--- | :--- | :--- |
| MFRC522 (NFC) | SDA / SS | GPIO 5 |
| MFRC522 (NFC) | SCK | GPIO 18 |
| MFRC522 (NFC) | MOSI | GPIO 23 |
| MFRC522 (NFC) | MISO | GPIO 19 |
| MFRC522 (NFC) | RST | GPIO 16 |
| PIR Sensor | Signal Output | GPIO 13 |
| TOF Sensor | SDA | GPIO 21 |
| TOF Sensor | SCL | GPIO 22 |
| Zone 1 Light | Signal | GPIO 25 |
| Zone 1 Fan | Signal | GPIO 26 |
| Zone 1 Servo | PWM | GPIO 4 |
| Zone 2 Light | Signal | GPIO 33 |
| Zone 2 Fan | Signal | GPIO 14 |
| Zone 2 Servo | PWM | GPIO 15 |
| Zone 3 Light | Signal | GPIO 32 |
| Zone 3 Fan | Signal | GPIO 27 |
| Zone 3 Servo | PWM | GPIO 17 |

*Note: Always power servos and fans from the external 15V power supply via IRF520 MOSFETs.*

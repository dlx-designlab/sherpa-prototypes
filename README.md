# Sherpa Prototypes

Prototype scripts for wearable heart rate broadcasting to multiple audience devices.

## System Overview

```
[TICKR HR Sensor] --(BLE)--> [M5Stack Core2 Broadcaster] --(ESP-NOW)--> [M5Stack Core2 Receiver x4]
                                        |
                                        +--(OSC/UDP)--> [Computer / DAW / TouchDesigner]
```

One **M5Stack Core2** acts as the central hub: it connects to a Wahoo TICKR heart rate monitor over BLE, then broadcasts the BPM data simultaneously to multiple receiver devices and optionally to a computer via OSC.

---

## Devices

### HR_Broadcaster_M5Core2
- Scans for a BLE heart rate sensor by name (`TICKR` by default, configurable)
- Connects and subscribes to heart rate notifications (BLE standard service `0x180D`)
- Broadcasts BPM as a raw `int32` via **ESP-NOW** (broadcast MAC `FF:FF:FF:FF:FF:FF`) to all receivers simultaneously
- Also sends BPM via **OSC** (`/heart_rate`) over WiFi UDP to a configurable IP (e.g. a laptop running TouchDesigner or Max/MSP)
- Display: shows BLE scan log on bottom half, BPM on right half once connected
- Battery level shown top-right, checked every 60 seconds

### HR_Receiver_M5StickC
- Receives BPM data via **ESP-NOW** — no pairing required, works automatically as long as both devices are on the same WiFi channel
- Display (320×240): full-screen color background indicating intensity zone, large BPM number centered
- Vibration motor pulses in sync with the heart rate (faster BPM = faster pulses)
- Battery level shown top-right, checked every 60 seconds
- **Hardware:** M5Stack Core2

---

## Heart Rate Zones (Receiver display color)

| BPM | Color | Zone |
|---|---|---|
| < 80 | Blue | Resting |
| 80–99 | Yellow | Light activity |
| 100–129 | Orange | Moderate |
| 130+ | Red | High intensity |

---

## Setup

1. Edit WiFi credentials in both sketches:
   ```cpp
   const char* WIFI_SSID     = "your_ssid";
   const char* WIFI_PASSWORD = "your_password";
   ```
2. Edit OSC destination in the broadcaster:
   ```cpp
   const char* OSC_HOST = "192.168.x.x"; // your computer's IP
   const int   OSC_PORT = 8000;
   ```
3. Flash `HR_Broadcaster_M5Core2` to the M5Stack Core2
4. Flash `HR_Receiver_M5StickC` to each receiver device
5. Power on the TICKR sensor, then power on the broadcaster — it will scan and connect automatically
6. Power on any number of receivers — they will start receiving BPM as soon as the broadcaster connects to the sensor

---

## Dependencies

- [M5Unified](https://github.com/m5stack/M5Unified)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [ESP32 Arduino OSC](https://github.com/CNMAT/OSC)
- ESP-NOW (built into ESP32 Arduino core, no install needed)


# Incubator Pro 🥚🌡️

A modern, web-controlled egg incubator system powered by ESP32, BLE (Bluetooth Low Energy), and a skeuomorphic web dashboard.

🔗 **Repository:** [manjisama1/incubator-web](https://github.com/manjisama1/incubator-web)

---

## 🌟 Key Features

* **Skeuomorphic Web Interface:** Metallic tactile dials, LCD telemetry, and interactive mechanical wheel for motor control.
* **Predictive PID Heating:** Filtered sensor readings with smooth PD duty-cycle relay switching for tight temperature regulation.
* **Manual Stepper Wheel:** Rotating interactive disk on the dashboard directly controls egg tray motor jogging in real time.
* **Independent Controls:** Auto heater control operates independently without disabling circulation fan or manual motor triggers.
* **Live Graphing:** Continuous temperature trend line rendered via Chart.js over Web Bluetooth.

---

## 📂 Repository Contents

| File | Description |
|---|---|
| `main.cpp` | ESP32 firmware (C++ / Arduino framework) handling DHT22, relays, AccelStepper motor, and BLE notifications. |
| `index.html` | Web Bluetooth control dashboard built with vanilla HTML, CSS, JavaScript, and Chart.js. |

---

## 🛠️ Hardware Requirements

* **Controller:** ESP32 Development Board
* **Temperature Sensor:** DHT22 (Pin `GPIO 4`)
* **Actuators:** 
  * Heater Relay (`GPIO 18`)
  * Fan Relay (`GPIO 19`)
* **Motor:** Stepper motor driven via ULN2003 or A4988 (`GPIO 13, 12, 14, 27`)

---

## 🚀 Quick Start Guide

### 1. Flash the ESP32
1. Open `main.cpp` in PlatformIO or Arduino IDE.
2. Install dependencies:
   * `DHT sensor library`
   * `AccelStepper`
3. Upload `main.cpp` to your ESP32 board.

### 2. Launch the Web Dashboard
1. Open `index.html` in a Web Bluetooth supported browser (**Google Chrome**, **Edge**, or **Brave**).
2. Click **CONNECT BLE**.
3. Select **Manjis_Incubator** from the Bluetooth pairing window.

> **Note:** Web Bluetooth requires a secure context (`https://`) or a local host server (`http://localhost`) to connect.

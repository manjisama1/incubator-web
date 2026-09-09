# Incubator Pro 🥚🌡️ 

A modern, web-controlled egg incubator system powered by ESP32, BLE (Bluetooth Low Energy), and a skeuomorphic web dashboard.

🔗 **Repository:** [manjisama1/incubator-web](https://github.com/manjisama1/incubator-web)

---

## 🌟 Key Features

* **Skeuomorphic Web Interface:** Metallic tactile dials, LCD telemetry, and interactive mechanical wheel for motor control.
* **Filtered Predictive Heating (PD Control):** Rate-limited/de-glitched DHT22 readings feed a Proportional-Derivative controller that computes a heater duty cycle, applied via a 10-second PWM relay window for tight, low-overshoot temperature regulation.
* **Manual Stepper Wheel:** Rotating interactive disk on the dashboard directly jogs the egg tray motor (28BYJ-48 + ULN2003) in real time via `JOG_FWD` / `JOG_REV` / `JOG_STOP`.
* **Non-Blocking Auto Tray Turning:** Configurable turn interval and duration run without blocking sensor reads, BLE notifications, or fan control.
* **Independent Controls:** Auto heater control, manual heater/fan overrides, and auto/manual tray turning all operate independently — toggling one doesn't force-disable the others unless explicitly coupled (e.g. disabling manual heater also disables auto mode).
* **Live Graphing:** Continuous temperature trend rendered on an HTML5 `<canvas>` with a smooth Catmull-Rom spline curve (no charting library dependency), gridlines with °C labels, and tap/hover-to-inspect tooltips showing exact value + timestamp per reading.
* **BLE JSON Telemetry:** ESP32 pushes live `temp`, `hum`, `rate`, `pwr`, `autoTemp`, `autoTurn`, `heater`, and `fan` state as JSON over BLE Notify.

---

## 📂 Repository Contents

| File | Description |
|---|---|
| `main.cpp` | ESP32 firmware (C++ / Arduino framework) handling DHT22 reads, PD-based heater duty cycling, relay control, non-blocking AccelStepper tray turning/jogging, and a BLE UART-style (Nordic UART Service UUIDs) GATT server for telemetry + commands. |
| `index.html` | Self-contained Web Bluetooth control dashboard — vanilla HTML, CSS, and JavaScript with a custom canvas-based smoothed line graph (no external charting library). |

---

## 🛠️ Hardware Requirements

* **Controller:** ESP32 Development Board
* **Temperature/Humidity Sensor:** DHT22 (`GPIO 4`)
* **Actuators:**
  * Heater Relay (`GPIO 18`)
  * Fan Relay (`GPIO 19`)
* **Motor:** 28BYJ-48 Stepper Motor + ULN2003 Driver (`GPIO 13, 12, 14, 27`, driven in HALF4WIRE mode via AccelStepper)
* **Power Supplies:** 12V DC Power Supply (for Fan & Heater) & 5V DC Power Supply / USB (for ESP32 & Stepper)

> **Note on relay logic:** Relays are active-LOW in firmware (`RELAY_ON = LOW`, `RELAY_OFF = HIGH`). If your relay module is active-HIGH, swap these defines in `main.cpp`.

---

## 🔌 Wiring & Connections

```text
               +-------------------+
               | 12V Power Supply  |
               | (+)           (-) |
               +--+-------------+--+
                  |             |
  +---------------+             +------------------------------+
  |                                                            |
 [Relay Module (Switches +12V High-Side)]                      |
  - COM 1 Pin -------------------+ (12V (+))                   |
  - COM 2 Pin -------------------+                             |
  - NO 1 Pin -----> [10x Resistor Array (+)]                   |
  - NO 2 Pin -----> [12V Fan Red Wire (+)]                     |
                                                               |
  [10x Resistor Array (-)] ------------------------------------+
  [12V Fan Black Wire (-)] ------------------------------------+

----------------------------------------------------------------

 [ESP32 & Low-Voltage Logic Side]
  - VIN Pin (5V) -> Relay VCC, DHT (+), ULN2003 VCC
  - GND Pin ------> Relay GND, DHT (-), ULN2003 GND (Common 5V GND)
  - GPIO 18 ------> Relay IN1 (Heater Control)
  - GPIO 19 ------> Relay IN2 (Fan Control)
  - GPIO 4  -------> DHT22 Data ("out")
  - GPIO 13 ------> ULN2003 IN1
  - GPIO 12 ------> ULN2003 IN2
  - GPIO 14 ------> ULN2003 IN3
  - GPIO 27 ------> ULN2003 IN4
```

---

## 🚀 Quick Start Guide

### 1. Flash the ESP32
1. Open `main.cpp` in PlatformIO or Arduino IDE.
2. Install dependencies:
   * `DHT sensor library` (Adafruit)
   * `AccelStepper`
   * ESP32 BLE Arduino libraries (`BLEDevice`, `BLEServer`, `BLEUtils`, `BLE2902`) — bundled with the ESP32 board package.
3. Upload `main.cpp` to your ESP32 board.
4. The device advertises over BLE as **`Manjis_Incubator`** using a Nordic UART–style service (`6E400001-...`), with a writable RX characteristic for commands and a notify TX characteristic for JSON telemetry.

### 2. Launch the Web Dashboard
1. Open `index.html` in a Web Bluetooth supported browser (**Google Chrome**, **Edge**, or **Brave**).
2. Click **CONNECT BLE**.
3. Select **Manjis_Incubator** from the Bluetooth pairing window.

> **Note:** Web Bluetooth requires a secure context (`https://`) or a local host server (`http://localhost`) to connect.

---

## 🎛️ BLE Command Reference

Sent as plain-text writes to the RX characteristic:

| Command | Effect |
|---|---|
| `PWR_ON` / `PWR_OFF` | Master power on/off (off also disables heater, fan, auto mode, and stops the motor) |
| `AUTO_ON` / `AUTO_OFF` | Enable/disable PD auto-temperature control |
| `HEATER_ON` / `HEATER_OFF` | Manual heater override (drops out of auto mode) |
| `FAN_ON` / `FAN_OFF` | Manual fan control (toggles "always on" fan mode) |
| `AUTO_TURN_ON` / `AUTO_TURN_OFF` | Enable/disable scheduled automatic tray turning |
| `SET_TEMP:<°C>` | Set target incubation temperature (e.g. `SET_TEMP:37.5`) |
| `SET_SPEED:<steps/s>` | Set stepper motor speed |
| `SET_TURN_DUR:<ms>` | Set duration of each auto tray turn |
| `SET_TURN_INT:<ms>` | Set interval between automatic tray turns |
| `JOG_FWD` / `JOG_REV` / `JOG_STOP` | Manual continuous jog control (from the dashboard's rotating disk) |
| `TURN_TRAY` | Force a single tray turn immediately |

Telemetry pushed back over the TX characteristic every ~2s (as JSON):
```json
{"temp":37.52,"hum":61.0,"rate":0.03,"pwr":true,"autoTemp":true,"autoTurn":true,"heater":true,"fan":true}
```

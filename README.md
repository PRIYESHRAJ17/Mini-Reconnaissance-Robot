# 🤖 Mini Reconnaissance Robot

A Wi-Fi controlled reconnaissance robot built with **Arduino Uno** and **ESP32-CAM**, featuring a live MJPEG camera stream, real-time sensor telemetry, and a custom military-style web dashboard — all accessible from any browser on the same network.

---

## ✨ Features

- 🎮 **Web-based control dashboard** — WASD keyboard + on-screen touch buttons
- 📷 **Live MJPEG camera stream** — real-time video from ESP32-CAM
- 🚧 **Auto obstacle detection** — dual ultrasonic sensors + IR sensors stop the robot automatically
- 🔥 **Flame detection** — alerts on dashboard when fire is detected
- 🔊 **Sound detection** — detects ambient sound events
- 📐 **MPU6050 IMU** — real-time tilt/orientation (X and Y axis)
- 💡 **Flash LED control** — toggle from dashboard or camera page
- ⚡ **Adjustable motor speed** — 10-step speed control (25–255 PWM)
- 🌐 **Dual HTTP server architecture** — commands on port 80, stream on same server via `/stream` route

---

## 🧰 Hardware Used

| Component | Purpose |
|---|---|
| Arduino Uno | Motor control, sensor reading, serial communication |
| ESP32-CAM | Wi-Fi server, camera streaming, web dashboard |
| L298N Motor Driver | Dual DC motor control |
| HC-SR04 × 2 | Ultrasonic distance sensors (left + right) |
| IR Sensor × 2 | Ground/edge detection (left + right) |
| MPU6050 | Gyroscope + accelerometer (IMU) |
| Flame Sensor | Fire detection |
| Sound Sensor | Ambient sound detection |
| Neo-6M GPS *(pending)* | GPS location tracking |

---

## 🏗️ Architecture

```
[ Browser ]
     |
     | HTTP (Wi-Fi)
     |
[ ESP32-CAM ] ──── Serial (9600 baud) ──── [ Arduino Uno ]
   Port 80                                       |
   /cmd        →  sends command char →      Motor Driver
   /speed      →  sends speed value  →      L298N → Motors
   /sensors    ←  reads JSON data    ←      Sensors
   /stream     →  MJPEG camera feed
   /camera     →  camera view page
```

**Key design decision:** Camera stream and command handling run on the same HTTP server (port 80) to prevent stream blocking. The `/stream` route uses chunked multipart response so it doesn't interfere with other endpoints.

---

## ⚙️ Pin Configuration

### Arduino Uno
| Pin | Component |
|---|---|
| D2, D3 | SoftwareSerial TX/RX to ESP32-CAM |
| D5 (ENA), D10 (ENB) | Motor speed (PWM) |
| D6, D7, D8, D9 | Motor direction (IN1–IN4) |
| D4 | IR Right |
| D11 | IR Left |
| D12 | Flame sensor |
| D13 | Sound sensor |
| A0, A1 | Ultrasonic Right (TRIG, ECHO) |
| A2, A3 | Ultrasonic Left (TRIG, ECHO) |
| SDA (A4), SCL (A5) | MPU6050 I2C |

### ESP32-CAM
| Pin | Component |
|---|---|
| GPIO14 (RXD2) | Serial RX from Arduino |
| GPIO15 (TXD2) | Serial TX to Arduino |
| GPIO4 | Flash LED |
| GPIO0–GPIO39 | Camera (OV2640) |

---

## 🚀 Setup & Usage

### 1. Clone the repo
```bash
git clone https://github.com/PRIYESHRAJ17/Mini-Reconnaissance-Robot.git
```

### 2. Configure Wi-Fi
In `ESP.ino`, replace the placeholders with your Wi-Fi credentials:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 3. Install Arduino Libraries
- `MPU6050` by Electronic Cats
- `SoftwareSerial` (built-in)
- `Wire` (built-in)
- ESP32 board package with `esp_camera.h` support

### 4. Flash the code
- Flash `ARDUINO.ino` to Arduino Uno
- Flash `ESP.ino` to ESP32-CAM (use FTDI adapter, GPIO0 to GND during flash)

### 5. Access the dashboard
- Open Serial Monitor on ESP32-CAM at 115200 baud
- Copy the IP address printed after Wi-Fi connects
- Open `http://<IP_ADDRESS>` in any browser on the same network

---

## 🎮 Controls

| Key | Action |
|---|---|
| W / ↑ | Forward |
| S / ↓ | Backward |
| A / ← | Turn Left |
| D / → | Turn Right |
| Space | Stop |
| + / - | Speed up / down |

Touch buttons are also available for mobile control.

---

## 📊 Sensor Dashboard

The web dashboard displays live telemetry every 300ms:

- **RIGHT DIST / LEFT DIST** — ultrasonic distance in cm
- **FLAME** — CLEAR or DETECTED
- **SOUND** — CLEAR or DETECTED
- **IR RIGHT / IR LEFT** — CLEAR or BLOCKED
- **MPU X / MPU Y** — accelerometer values with NORMAL/TILTED status
- **OBSTACLE ALERT** — red banner appears when path is blocked

---

## 🔜 Upcoming

- [ ] GPS integration (Neo-6M) — live coordinates on dashboard
- [ ] GPS outdoor testing
- [ ] Night vision mode improvements

---

## 👤 Author

**Priyesh Raj**  
1st Year B.E. CSE (Data Science) — DSATM, Bengaluru  
[GitHub](https://github.com/PRIYESHRAJ17)

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

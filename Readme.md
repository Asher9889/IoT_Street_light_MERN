## 🌟 Who is who?

| 📌 Role        | ✅ Who/What acts as this?                                             |
| -------------- | -------------------------------------------------------------------- |
| **Publisher**  | 👉 Your Node.js backend — It publishes MQTT messages when API is hit |
| **Broker**     | 👉 Your MQTT broker (e.g. Mosquitto / HiveMQ) — It routes messages   |
| **Subscriber** | 👉 The ESP32 device — It listens (subscribes) to its command topic   |

---

## 📊 IoT System Development Plan (ESP32 + MQTT + Node.js + MongoDB)
| 🔢 Step | 🧠 Task                       | 🔧 Action / Description                                                                                                                                                                    | 🧱 Tools / Technologies         |
| ------- | ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------- |
| 1️⃣     | **Device ID Strategy**        | Manually define a unique `deviceId` like `device001`                                                                                                                                       | Manual naming convention        |
| 2️⃣     | **Device Registration Panel** | Create a simple web UI to register devices (store `deviceId`, relay count, etc.)                                                                                                           | React / HTML + REST API         |
| 3️⃣     | **MongoDB Schema Setup**      | Create `devices` collection with schema:<br>`{deviceId, name, relayCount, status, lastSeen}`                                                                                               | MongoDB + Mongoose              |
| 4️⃣     | **Backend API Setup**         | REST API to:<br>✅ Register device<br>✅ Get device status<br>✅ Update command<br>✅ History (optional)                                                                                       | Node.js + Express + Mongoose    |
| 5️⃣     | **Setup MQTT Broker**         | Install and configure **Mosquitto** broker on your server                                                                                                                                  | Mosquitto                       |
| 6️⃣     | **Integrate MQTT in Node.js** | Use `mqtt` npm package to:<br>📤 Publish to `iot/devices/<deviceId>/cmd`<br>📥 Subscribe to `iot/devices/<deviceId>/status`                                                                | MQTT + Node.js                  |
| 7️⃣     | **ESP32 MQTT Firmware**       | Flash code to ESP32:<br>✅ Read `deviceId` from flash or hardcoded<br>✅ Connect to GPRS<br>✅ Connect to MQTT<br>✅ Subscribe to `iot/devices/<deviceId>/cmd`<br>✅ Control relays accordingly | C++ with TinyGSM + PubSubClient |
| 8️⃣     | **Bi-Directional Messaging**  | ESP32 publishes relay status to:<br>📡 `iot/devices/<deviceId>/status`<br>Backend parses and stores in DB                                                                                  | MQTT Publish from ESP32         |
| 9️⃣     | **Web Dashboard**             | UI shows:<br>✅ List of devices<br>✅ Toggle relays<br>✅ View status<br>✅ Add/remove devices                                                                                                 | ReactJS + REST + MQTT           |
| 🔟      | **Security Measures**         | - Secure MQTT (username/password)<br>- Auth for REST API<br>- Device auth (optional)                                                                                                       | TLS, JWT, API keys              |
| 🔁      | **Scale to 100+ Devices**     | Devices follow same topic pattern, UI/dashboard lists & controls each individually                                                                                                         | MQTT topic hierarchy            |
| 🧪      | **Test System End-to-End**    | 1. Register device<br>2. Flash ESP32<br>3. Toggle from dashboard<br>4. See relay respond                                                                                                   | Localhost/dev to Live           |

---

🧠 MQTT Architecture Overview (Full Picture)


┌────────────┐        ┌─────────────────────────┐       ┌────────────────┐
│ ESP32 #001 │◄──────►│     MQTT BROKER (Your PC)│◄────►│  Node.js Server │
└────┬───────┘        └────────────┬────────────┘       └────┬───────────┘
     │                             │                           │
     ▼                             ▼                           ▼
Sub: iot/devices/device001/cmd     DB Save Cmd         Web UI publish msg
Pub: iot/devices/device001/status        └────── REST API ──────►


## 📝 Next steps 
- Setting up Mosquitto broker on Windows
     - Download and install on the window server
          ``` https://mosquitto.org/download/```

- Configure TLS + auth

- Open the broker securely to the world

- Design topic structure + ACLs

- Node.js code for MQTT publish/subscribe

- Web dashboard integration
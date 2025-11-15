✅ COMPLETE SYSTEM FLOW: FROM CLICK TO BULB ACTION

| **Step** | **What Happens**                                                             | **Component**         | **Skills/Tech You Need**                 |
| -------- | ---------------------------------------------------------------------------- | --------------------- | ---------------------------------------- |
| 1️⃣      | User opens a webpage and clicks "ON" or "OFF" button                         | Frontend (HTML/JS)    | 🔸 Basic HTML & JavaScript               |
| 2️⃣      | Button click sends `POST /set-command` to the backend server                 | Web Server (API)      | 🔸 Node.js, Express, HTTP, REST API      |
| 3️⃣      | Server receives the command and stores `"ON"` or `"OFF"`                     | In-memory or database | 🔸 Node.js state / MongoDB (optional)    |
| 4️⃣      | ESP32 + SIM900A module sends `GET /latest-command` to server every 1s        | ESP32 + SIM900A       | 🔸 AT Commands, C++, UART communication  |
| 5️⃣      | Server responds with `"ON"` or `"OFF"`                                       | Web Server            | 🔸 Understanding of HTTP response        |
| 6️⃣      | ESP32 parses the response and turns **GPIO 5 HIGH/LOW** to control the relay | ESP32 Microcontroller | 🔸 Arduino C++, GPIO control             |
| 7️⃣      | Relay switches the 230V AC power to bulb ON/OFF                              | Relay module          | 🔸 Electrical wiring (basic electronics) |
| 8️⃣      | Bulb responds and turns ON or OFF in real-time                               | Bulb + power supply   | 🔸 Safe handling of AC components        |





🧠 SUMMARY OF MODULES + YOUR ROLE

| **Module**              | **Your Responsibility**                            | **Skill Needed**              |
| ----------------------- | -------------------------------------------------- | ----------------------------- |
| 🖥️ Frontend (Website)  | Create buttons + send HTTP requests                | HTML, JS (fetch API)          |
| 🌐 Backend (Server)     | Receive/store commands, expose REST APIs           | Node.js, Express              |
| 📶 GSM Module (SIM900A) | Send HTTP GET requests over GPRS using AT Commands | AT Commands, UART serial comm |
| 🧠 ESP32                | Parse response, control GPIO for relay             | Arduino C++, GPIO basics      |
| ⚡ Relay + Bulb          | Physically switch AC line to bulb                  | Electronics basics            |


| Think of it like this:                    |
| ----------------------------------------- |
| **ESP32** = Brain 🧠                      |
| **SIM900A** = SIM-based Internet Modem 🌐 |


| **Component**                                           | **Details / Code**                                               | **Purpose / Description**                                                        |
| ------------------------------------------------------- | ---------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| `HardwareSerial sim900(1);`                             | Declares `Serial1` UART port (UART1)                             | Tells ESP32 that we’re using a second UART interface to communicate with SIM900A |
| `sim900.begin(9600, SERIAL_8N1, SIM900_RX, SIM900_TX);` | `9600` baud rate with GPIO16 (RX), GPIO17 (TX)                   | Initializes serial connection between ESP32 and SIM900A                          |
| `#define SIM900_RX 16`                                  | Connect ESP32 GPIO16 to **SIM900A TX**                           | This pin receives data from SIM900A                                              |
| `#define SIM900_TX 17`                                  | Connect ESP32 GPIO17 to **SIM900A RX**                           | This pin sends data to SIM900A                                                   |
| `sendAT("AT+...")`                                      | `sim900.println(...)` is used inside this function               | Sends AT commands to the SIM module                                              |
| `sim900.read()` / `available()`                         | Used to read responses from SIM900A (inside loops)               | ESP32 reads whatever SIM900A replies after AT command                            |
| `setupGPRS()`                                           | Function calling `AT+CGATT`, `AT+SAPBR`, etc.                    | Establishes mobile data connection using SIM900A GPRS                            |
| `getCommandFromServer()`                                | Sends HTTP GET request via AT commands (polling server)          | Connects to your server and receives `"ON"` or `"OFF"`                           |
| `digitalWrite(RELAY_PIN, HIGH/LOW)`                     | Controls the relay based on server command                       | Turns the bulb ON or OFF via relay module                                        |
| `delay(1000)`                                           | Waits 1 second before next server poll                           | Prevents hammering server too frequently                                         |
| Power Connection                                        | SIM900A **must** be powered with **5V, 2A** via external adapter | ESP32 cannot supply enough current on its own                                    |
| Baud Rate                                               | `9600`                                                           | Standard baud rate for SIM900A                                                   |
| GND                                                     | Common GND between ESP32 and SIM900A                             | Ensures proper communication                                                     |


| ✅ Checkpoint                                     | Confirmed? |
| ------------------------------------------------ | ---------- |
| ESP32 is wired correctly to SIM900A              | ✅  |
| SIM900A is powered via 5V, 2A supply (not ESP32) | ✅      |
| SIM card (Airtel IoT) is inserted into SIM900A   | ✅     |
| Antenna is attached to SIM900A                   | ✅     |
| `RX`, `TX` connected properly (crossed)          | ✅      |
| Arduino IDE is ready, board = `ESP32 Dev Module` | ✅     |
| Port (e.g. `/dev/ttyUSB0`) is selected           | ✅ |
| You’ve replaced this URL:                        |            |




sudo chmod a+rw /dev/ttyUSB0

ngrok http --scheme=http 5000

📍 Left → Center → Right
   B      C       E
(Base, Collector, Emitter)


wiring is done. red and green lights are on. esp32 tx connect with RXD3.3V and esp32 rx connect with TXD 3.3v. esp gnd connect with gnd of sim900a. esp gnd connect with one relay gnd. relay in pin connects with gpio5. relay vcc connect with esp32 vin.
why still not working even relay lights are on.


---
## 🧱 1. Database Schema (MySQL or MongoDB)

| Table / Collection | Fields                                                                   | Description                           |
| ------------------ | ------------------------------------------------------------------------ | ------------------------------------- |
| `users`            | `id`, `name`, `email`, `passwordHash`, `role`, `created_at`              | Stores registered users               |
| `devices`          | `id`, `user_id`, `imei`, `sim_number`, `label`, `location`, `created_at` | One device per user                   |
| `relays`           | `id`, `device_id`, `relay_number`, `status`, `label`                     | Each GPIO relay connected to a device |
| `commands`         | `id`, `device_id`, `relay_number`, `command`, `issued_by`, `timestamp`   | Logs user commands like ON/OFF        |
| `logs`             | `id`, `device_id`, `type`, `message`, `timestamp`                        | Debug/Device logs                     |

---

## 🚦 2. System Flow Overview

| Step | Action                                                    |
| ---- | --------------------------------------------------------- |
| 1    | Power on → SIM900A connects to GPRS                       |
| 2    | Periodically send `GET /api/device/:id/latest-command`    |
| 3    | Parse JSON: `{ [relay0]: "ON", [relay1]: "OFF", ... }`    |
| 4    | Update relays accordingly using `digitalWrite()`          |
| 5    | Optionally send heartbeat or status update back to server |

## 🧑‍💻 User Side (Web Dashboard)

| Step | Action                                                              |
| ---- | ------------------------------------------------------------------- |
| 1    | User registers and logs in (JWT token issued)                       |
| 2    | User links device using IMEI or device key                          |
| 3    | Dashboard shows current status of all relays                        |
| 4    | User clicks ON/OFF → sends `POST /api/command`                      |
| 5    | Server updates command in DB, which is fetched by device next cycle |


## 🖥️ 3. Backend (Node.js + Express + JWT + DB)

| API Route                        | Method | Purpose                                 |
| -------------------------------- | ------ | --------------------------------------- |
| `/api/auth/register`             | POST   | Register user                           |
| `/api/auth/login`                | POST   | Login and return JWT                    |
| `/api/devices`                   | GET    | List all devices for a user             |
| `/api/devices`                   | POST   | Register new device (IMEI etc.)         |
| `/api/relays/:deviceId`          | GET    | Get status of relays for a device       |
| `/api/command`                   | POST   | Update relay command (ON/OFF)           |
| `/api/device/:id/latest-command` | GET    | Used by ESP32 to get relay command JSON |

## 🛠️ 4. Frontend Stack Options

| Stack         | Tools / Techs            |
| ------------- | ------------------------ |
| Frontend      | React + Tailwind CSS     |
| Auth          | JWT + LocalStorage       |
| Charts        | Recharts / Chart.js      |
| UI Components | shadcn/ui or Material UI |
| Deployment    | Netlify / Vercel         |

## 🔐 5. Security & Scaling

| Aspect        | Action                                                 |
| ------------- | ------------------------------------------------------ |
| JWT Auth      | Short-lived access + long-lived refresh tokens         |
| Rate Limiting | For routes accessed by ESP32 using IP limits or tokens |
| Validation    | Joi / Zod to validate request bodies                   |
| Error Logs    | Use `winston`, `morgan`, or Sentry                     |
| TLS (SSL)     | Always use HTTPS (ngrok, NGINX, or Cloudflare tunnel)  |

## 🧪 6. Testing & Deployment

| Stage        | Tools                               |
| ------------ | ----------------------------------- |
| Unit Testing | Jest or Mocha                       |
| API Testing  | Postman / Insomnia                  |
| CI/CD        | GitHub Actions + PM2 + NGINX        |
| Hosting      | Railway / Render / VPS + Plesk      |
| DB Hosting   | PlanetScale (MySQL) / MongoDB Atlas |

## 🛠️ 7. Bonus Features for Production

| Feature           | Description                              |
| ----------------- | ---------------------------------------- |
| Scheduling        | Turn ON/OFF at specific times using cron |
| Notifications     | Email/SMS on failure or manual override  |
| Grouping          | Group relays by room/floor/zone          |
| Offline Detection | Flag when a device misses N polls        |
| Role-based Access | Admin, Operator, Viewer                  |



# MQTT Protocol

sudo dnf install mosquitto-clients

📦 Required Libraries in Arduino IDE
Install these via Library Manager:

- TinyGSM
- PubSubClient
- ArduinoJson

---
## 📘 MQTT Full Overview

| Category              | Details                                                                    |
| --------------------- | -------------------------------------------------------------------------- |
| **Full Form**         | MQTT = Message Queuing Telemetry Transport                                 |
| **Type**              | Lightweight Publish/Subscribe Protocol                                     |
| **Transport Layer**   | TCP (usually port **1883** for non-SSL, **8883** for SSL)                  |
| **Ideal For**         | IoT, embedded systems, low-bandwidth or unstable networks                  |
| **Protocol Features** | - Small code footprint<br>- Low power consumption<br>- Real-time messaging |
| **Broker**            | Central server that manages message routing (e.g., Mosquitto)              |
| **Client Types**      | Publisher (sends messages) and Subscriber (receives messages)              |
| **Message Structure** | Topic (e.g., `home/bulb1`) + Payload (e.g., `"ON"`)                        |
| **QoS Levels**        | 0 (At most once), 1 (At least once), 2 (Exactly once)                      |
| **Security**          | Username/Password + TLS/SSL                                                |
| **Popular Brokers**   | Mosquitto, EMQX, HiveMQ, VerneMQ, AWS IoT Core                             |
| **Client Libraries**  | `paho-mqtt`, `mqtt.js`, `PubSubClient` (ESP8266/ESP32), `async-mqtt`       |
---

## 🎯 Why Use MQTT Over HTTP

| MQTT Feature                     | HTTP Limitation / Advantage of MQTT                        |
| -------------------------------- | ---------------------------------------------------------- |
| **Bi-directional Communication** | HTTP is one-way per request; MQTT allows real-time two-way |
| **Lightweight**                  | MQTT has lower packet overhead                             |
| **Faster**                       | No need to open/close connection repeatedly                |
| **Persistent Session**           | Keeps the connection alive                                 |
| **Offline Support**              | MQTT can queue messages for delivery later                 |
| **Low Bandwidth Usage**          | Perfect for GSM or mobile networks                         |

---

## 🛠️ How to Use MQTT (Step-by-Step)

| Step | Description                | Example                                             |
| ---- | -------------------------- | --------------------------------------------------- |
| 1    | Install Broker             | `sudo dnf install mosquitto`                        |
| 2    | Start Broker               | `mosquitto -v`                                      |
| 3    | Install Client Tools       | `sudo dnf install mosquitto-clients`                |
| 4    | Subscribe to a Topic       | `mosquitto_sub -h localhost -t home/relay1`         |
| 5    | Publish a Message          | `mosquitto_pub -h localhost -t home/relay1 -m "ON"` |
| 6    | Use MQTT Library in ESP32  | Use `PubSubClient` library in Arduino               |
| 7    | Write ESP32 Code           | Connect to WiFi + MQTT, then publish/subscribe      |
| 8    | Handle Commands via Topics | Topic: `home/bulbs/1`, Payload: `"ON"` or `"OFF"`   |





/src
 ├── main.cpp
 ├── mqtt/
 │    ├── mqtt_client.cpp
 │    ├── mqtt_handlers.cpp
 │    └── mqtt_utils.h
 ├── lora/
 │    ├── lora_txrx.cpp
 │    └── lora_packets.h
 └── config.h


GATEWAy Subs Topic:

iot/gateway/<gatewayId>/config/set
iot/gateway/<gatewayId>/node/assign
iot/gateway/<gatewayId>/node/+/config/set
iot/gateway/<gatewayId>/control


iot/gateway/<gatewayId>/node/<nodeId>/control : Control the light
payload: {
  "type": "node_control",
  "gatewayId": "GW-4",
  "nodeId": "nodeCC29490B65F4",
  "action": "ON"
}

# Flow to control node light
| Step                  | Direction | Topic                                            | Payload                                                                                                  | Effect                                                                                                |
| --------------------- | --------- | ------------------------------------------------ | -------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| **Backend → Gateway** | MQTT      | `iot/gateway/GW-4/node/nodeCC29490B65F4/control` | `{ "type":"node_control","gatewayId":"GW-4","nodeId":"nodeCC29490B65F4","action":"ON","mode":"MANUAL" }` | Gateway sends LoRa `pktType=0x07` → node turns light **ON**, mode=MANUAL_ON                           |
| **Backend → Gateway** | MQTT      | same                                             | `{ "action":"OFF","mode":"MANUAL" }`                                                                     | Gateway sends `pktType=0x07` → node turns light **OFF**, mode=MANUAL_OFF                              |
| **Backend → Gateway** | MQTT      | same                                             | `{ "action":"AUTO","mode":"AUTO" }`                                                                      | Gateway sends `pktType=0x07` with `lightOn=false` → node switches to AUTO mode (resumes RTC schedule) |


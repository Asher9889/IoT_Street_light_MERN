/* Gateway Firmware - backend-driven dynamic config
   Requires: ArduinoJson, SPIFFS/LittleFS, LoRa, TinyGsm, PubSubClient
   Notes:
     - Backend must listen on "iot/gateway/register" and reply on "iot/gateway/<deviceId>/config/set"
     - After receiving full config (must contain gatewayId), gateway persists config and switches topics.
*/

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <FS.h>
#include <SPIFFS.h>
#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h> // optional (for MAC if needed)

// ---------------- LoRa Pins (adjust to your board) ----------------
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

// ---------------- Default LoRa Configuration ----------------
#define DEFAULT_LORA_FREQ 433000000UL
#define DEFAULT_LORA_SF   7
#define DEFAULT_LORA_BW   125000UL
#define DEFAULT_LORA_CR   5

// ---------------- GSM Pins ----------------
#define MODEM_RX 16
#define MODEM_TX 17
#define DEFAULT_APN "airtelgprs.com"

String APN = DEFAULT_APN;

// ---------------- Filesystem ----------------
const char *CONFIG_PATH = "/gateway_config.json";

// ---------------- MQTT default/fallback broker ----------------
const char* DEFAULT_BROKER = "103.20.215.109";
const int DEFAULT_PORT = 1883;

// ---------------- Hardware objects ----------------
HardwareSerial sim900(1);
TinyGsm modem(sim900);
TinyGsmClient gsmClient(modem);
PubSubClient mqtt(gsmClient);

// ---------------- LEDs ----------------
#define LED_POWER 2
#define LED_CONN  4
#define LED_DATA  15

// ---------------- timers ----------------
unsigned long lastBeacon = 0;
const unsigned long BEACON_INTERVAL = 8000UL; // 8s
unsigned long lastTelemetry = 0;
const unsigned long TELEMETRY_INTERVAL = 60000UL; // 60s
unsigned long lastMqttReconnect = 0;

// ---------------- GPRS fail monitoring ----------------

const unsigned long GPRS_CHECK_INTERVAL = 1000UL;     // Check every 1 second
const unsigned long GPRS_RECONNECT_DELAY = 5000UL;    // Wait 5s before retrying after failure
unsigned long lastGprsCheck = 0;
unsigned long lastGprsAttempt = 0;
unsigned long gprsFailStart = 0;
bool gprsWasConnected = false;

//----------------- FOR QUEUE ----------------
struct PendingCommand {
    char nodeId[24];
    char gatewayId[24];
    bool lightOn;
    unsigned long sentAt;
    int attempts;
    bool waitingAck;
    bool active;
};


// --- Robust GPRS Connection Management ---
bool connectGPRS(bool fullRestart = false) {
  static uint8_t retries = 0;
  const uint8_t MAX_RETRIES = 10;

  if (fullRestart || retries >= MAX_RETRIES) {
    Serial.println("[SIM900A] Full modem restart...");
    modem.restart();
    delay(3000);  // Give modem time to boot
    retries = 0;
  }

  if (!modem.isNetworkConnected()) {
    Serial.println("[SIM900A] Waiting for network...");
    if (!modem.waitForNetwork(30000L)) {  // 30s max
      retries++;
      return false;
    }
  }

  if (modem.isGprsConnected()) modem.gprsDisconnect();
  delay(1000);

  Serial.printf("[SIM900A] Connecting GPRS (APN=%s)...\n", APN.c_str());
  if (modem.gprsConnect(APN.c_str(), "", "")) {
    Serial.println("[SIM900A] GPRS connected!");
    retries = 0;
    return true;
  } else {
    Serial.println("[SIM900A] GPRS connect failed.");
    retries++;
    return false;
  }
}

bool ensureGprsConnection() {
  if (modem.isNetworkConnected() && modem.isGprsConnected()) return true;
  Serial.println("[SIM900A] GPRS not connected, attempting recovery...");
  return connectGPRS();
}



// ---------------- Config structures ----------------
struct NodeInfo {
  char nodeId[24];
  uint8_t onHour;
  uint8_t onMin;
  uint8_t offHour;
  uint8_t offMin;
  uint8_t configVersion;
};

#define MAX_NODES 50
NodeInfo nodeList[MAX_NODES];
size_t nodeCount = 0;

// Gateway-level config (populated from SPIFFS or backend)
String GATEWAY_ID = ""; // logical id e.g. "GW-1" (empty => not yet provisioned)
uint32_t LORA_FREQUENCY = DEFAULT_LORA_FREQ;
uint8_t  LORA_SF = DEFAULT_LORA_SF;
uint32_t LORA_BW = DEFAULT_LORA_BW;
uint8_t  LORA_CR = DEFAULT_LORA_CR;
String MQTT_BROKER = DEFAULT_BROKER;
int MQTT_PORT = DEFAULT_PORT;
String backendDeviceTopicBase; // iot/gateway/<deviceId>/
String backendGatewayTopicBase; // iot/gateway/<gatewayId>/

int configVersion = 0;
String deviceIdStr; // device unique identifier derived from efuse mac

// topics (updated dynamically)
String topic_device_config_set;   // iot/gateway/<deviceId>/config/set
String topic_device_register;     // iot/gateway/<deviceId>/register
String topic_gateway_config_set;  // iot/gateway/<gatewayId>/config/set
String topic_gateway_config_get;  // iot/gateway/<gatewayId>/config/get
String topic_gateway_node_assign; // iot/gateway/<gatewayId>/node/assign
String topic_gateway_node_config; // iot/gateway/<gatewayId>/node/+/config/set (subscribe)
String topic_gateway_status;      // iot/gateway/<gatewayId>/status
String topic_gateway_control;     // iot/gateway/<gatewayId>/control
String topic_generic_register = "iot/gateway/register"; // global backend listen
String topic_node_control; // iot/gateway/<gatewayId>/node/+/control


// ---------------- Packed structs used over LoRa (compact binary packets) ----------------
struct __attribute__((packed)) BeaconPkt {
  uint8_t pktType; // 0x01
  uint32_t uptime_s;
};
struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType; // 0x02
  char nodeId[24];
  uint8_t fwVersion;
  uint32_t uptime_s;
};
struct __attribute__((packed)) AssignPkt {
  uint8_t pktType; // 0x03
  char nodeId[24];
};
struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType; // 0x04
  char nodeId[24];
  char gatewayId[24];
  uint8_t onHour;
  uint8_t onMin;
  uint8_t offHour;
  uint8_t offMin;
  uint8_t cfgVer;
  uint32_t regIntervalMs;
  uint32_t statusIntervalMs;
};
struct __attribute__((packed)) PolePacket {
  char nodeId[24];
  char gatewayId[24];
  bool lightState;
  bool fault;
  uint8_t hour;
  uint8_t minute;
  int rssi;
  int snr;
};
struct __attribute__((packed)) AckPkt {
  uint8_t pktType;   // 0x06
  char nodeId[24];
  uint8_t cfgVer;
};
struct __attribute__((packed)) ControlPkt {
  uint8_t pktType; // 0x07
  char nodeId[24];
  char gatewayId[24];
  bool lightOn;
};
struct __attribute__((packed)) LoRaConfigPkt {
  uint8_t pktType;  // 0x08
  uint32_t freq;
  uint8_t sf;
  uint32_t bw;
  uint8_t cr;
};


// ---------------- Helpers ----------------
void blinkDataLED(int duration = 50) {
  digitalWrite(LED_DATA, HIGH);
  delay(duration);
  digitalWrite(LED_DATA, LOW);
}
volatile bool isLoRaBusy = false;  // global TX flag

void sendLoRaPacket(const uint8_t* data, size_t len, bool silent = false) {
  if (isLoRaBusy) {
    Serial.println("[WARN] LoRa TX requested while busy, skipping...");
    return;
  }

  isLoRaBusy = true;  // mark TX in progress
  LoRa.idle();        // ensure chip ready for TX
  LoRa.beginPacket();
  LoRa.write(data, len);
  LoRa.endPacket();
  delay(400);          // let TX complete fully

  LoRa.receive();     // re-enable RX mode
  if (!silent) Serial.println("[LORA] Back to RX mode");

  blinkDataLED(20);
  isLoRaBusy = false; // TX complete
}



String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%012llX", (unsigned long long)chipid);
  return String("device") + String(id);
}

void applyLoRaParamsAndStart() {
  // End any previous session
  LoRa.end();
  delay(200);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[LORA] init FAILED");
    return;
  }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.enableCrc();
  LoRa.receive();
  Serial.printf("[LORA] Started (Freq=%lu Hz, SF=%d, BW=%lu Hz, CR=4/%d)\n",
                (unsigned long)LORA_FREQUENCY, LORA_SF, (unsigned long)LORA_BW, LORA_CR);
}

// ----------------- queue to store commands----------------------
#define MAX_PENDING 10
PendingCommand cmdQueue[MAX_PENDING];
int currentCmdIndex = -1;

void initPendingQueue() {
    for (int i=0; i<MAX_PENDING; i++) cmdQueue[i].active = false;
}

void enqueuePendingCommand(const JsonDocument& doc) {
  for (int i=0; i<MAX_PENDING; i++) {
    if (!cmdQueue[i].active) {

      PendingCommand &c = cmdQueue[i];
      c.active = true;

      strncpy(c.nodeId, doc["nodeId"], sizeof(c.nodeId)-1);
      strncpy(c.gatewayId, doc["gatewayId"], sizeof(c.gatewayId)-1);

      const char* action = doc["action"] | "";
      c.lightOn = (strcasecmp(action, "ON") == 0);

      c.sentAt = 0;
      c.attempts = 0;
      c.waitingAck = false;

      Serial.printf("[QUEUE] Command queued for %s (ON=%d)\n", c.nodeId, c.lightOn);
      return;
    }
  }
  Serial.println("[QUEUE] FULL — cannot enqueue!");
}

void sendPendingCommandToLora(PendingCommand &c) {
    ControlPkt pkt;
    pkt.pktType = 0x07;
    strncpy(pkt.nodeId, c.nodeId, sizeof(pkt.nodeId)-1);
    strncpy(pkt.gatewayId, c.gatewayId, sizeof(pkt.gatewayId)-1);
    pkt.lightOn = c.lightOn;

    sendLoRaPacket((uint8_t*)&pkt, sizeof(pkt));

    c.sentAt = millis();
    c.attempts++;
    c.waitingAck = true;

    Serial.printf("[CMD] Sent → node=%s try=%d\n", c.nodeId, c.attempts);
}

void processPendingCommands() {

    // If a command is waiting for ACK → check timeout
    if (currentCmdIndex >= 0) {
        PendingCommand &c = cmdQueue[currentCmdIndex];

        if (c.waitingAck) {
            unsigned long now = millis();

            if (now - c.sentAt > 500) {   // 500ms timeout
                if (c.attempts < 2) {
                    Serial.println("[CMD] ACK timeout — retrying...");
                    sendPendingCommandToLora(c);
                } else {
                    Serial.println("[CMD] FAILED after retries.");
                    c.active = false;
                    c.waitingAck = false;
                    currentCmdIndex = -1;
                }
            }
        }
        return;   
    }

    // find next command
    for (int i=0; i<MAX_PENDING; i++) {
        if (cmdQueue[i].active) {
            currentCmdIndex = i;
            sendPendingCommandToLora(cmdQueue[i]);
            return;
        }
    }
}


// ---------------- Storage: save/load JSON config ----------------
bool loadConfigFromSPIFFS() {
  if (!SPIFFS.exists(CONFIG_PATH)) {
    Serial.println("[CONFIG] no config file in SPIFFS");
    return false;
  }
  File f = SPIFFS.open(CONFIG_PATH, FILE_READ);
  if (!f) {
    Serial.println("[SPIFFS] failed open config");
    return false;
  }
  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.print("[CONFIG] deserialize error: ");
    Serial.println(err.c_str());
    return false;
  }

  // parse known fields (defensive defaults)
  GATEWAY_ID = String(doc["gatewayId"] | "");
  LORA_FREQUENCY = doc["lora"]["frequency"] | LORA_FREQUENCY;
  LORA_SF = doc["lora"]["spreadingFactor"] | LORA_SF;
  LORA_BW = doc["lora"]["bandwidth"] | LORA_BW;
  LORA_CR = doc["lora"]["codingRate"] | LORA_CR;
  APN = String(doc["apn"] | APN);
  MQTT_BROKER = String(doc["mqtt"]["broker"] | MQTT_BROKER);
  MQTT_PORT = doc["mqtt"]["port"] | MQTT_PORT;
  configVersion = doc["configVersion"] | configVersion;

  // load nodes array (optional)
  nodeCount = 0;
  if (doc.containsKey("nodes") && doc["nodes"].is<JsonArray>()) {
    JsonArray nodes = doc["nodes"].as<JsonArray>();
    for (JsonObject n : nodes) {
      if (nodeCount >= MAX_NODES) break;
      String nid = String(n["nodeId"] | "");
      strncpy(nodeList[nodeCount].nodeId, nid.c_str(), sizeof(nodeList[nodeCount].nodeId)-1);
      nodeList[nodeCount].onHour = n["config"]["onHour"] | 0;
      nodeList[nodeCount].onMin  = n["config"]["onMin"]  | 0;
      nodeList[nodeCount].offHour = n["config"]["offHour"] | 0;
      nodeList[nodeCount].offMin  = n["config"]["offMin"] | 0;
      nodeList[nodeCount].configVersion = n["configVersion"] | 0;
      nodeCount++;
    }
  }

  // setup topic bases if gatewayId present
  if (GATEWAY_ID.length() > 0) {
    backendGatewayTopicBase = "iot/gateway/" + GATEWAY_ID + "/";
    topic_gateway_config_set = backendGatewayTopicBase + "config/set";
    topic_gateway_config_get = backendGatewayTopicBase + "config/get";
    topic_gateway_node_assign = backendGatewayTopicBase + "node/assign";
    topic_gateway_node_config = backendGatewayTopicBase + "node/+/config/set";
    topic_gateway_status = backendGatewayTopicBase + "status";
    topic_gateway_control = backendGatewayTopicBase + "control";
    topic_node_control = backendGatewayTopicBase + "node/+/control";
  }

  Serial.printf("[CONFIG] loaded gatewayId=%s nodes=%d freq=%lu broker=%s:%d\n",
                GATEWAY_ID.c_str(), nodeCount, (unsigned long)LORA_FREQUENCY, MQTT_BROKER.c_str(), MQTT_PORT);
  return true;
}

bool saveConfigToSPIFFS(const JsonDocument& doc) {
  // remove old file then write
  SPIFFS.remove(CONFIG_PATH);
  File f = SPIFFS.open(CONFIG_PATH, FILE_WRITE);
  if (!f) {
    Serial.println("[SPIFFS] failed to open config for writing");
    return false;
  }
  if (serializeJson(doc, f) == 0) {
    Serial.println("[SPIFFS] failed to write config");
    f.close();
    return false;
  }
  f.close();
  Serial.println("[CONFIG] saved config to SPIFFS");
  return true;
}

// ---------------- MQTT / Backend handling ----------------
String extractNodeIdFromTopic(const char* topic) {
  // naive split: return the token after "node/"
  String t = String(topic);
  int idx = t.indexOf("/node/");
  if (idx == -1) return String("");
  int start = idx + 6;
  int next = t.indexOf('/', start);
  if (next == -1) next = t.length();
  return t.substring(start, next);
}

void handleNodeConfig(const JsonDocument& doc, const char* topic) {
  ConfigPkt pkt;
  pkt.pktType = 0x04;

  const char* nodeIdPayload = doc["nodeId"] | "";
  String nodeIdStr = nodeIdPayload;
  if (nodeIdStr.length() == 0) {
    nodeIdStr = extractNodeIdFromTopic(topic);
  }
  strncpy(pkt.nodeId, nodeIdStr.c_str(), sizeof(pkt.nodeId)-1);
  pkt.nodeId[sizeof(pkt.nodeId)-1] = '\0';

  const char* gw = doc["gatewayId"] | "";
  strncpy(pkt.gatewayId, gw, sizeof(pkt.gatewayId)-1);
  pkt.gatewayId[sizeof(pkt.gatewayId)-1] = '\0';

  pkt.onHour = doc["schedule"]["onHour"] | 0;
  pkt.onMin  = doc["schedule"]["onMin"]  | 0;
  pkt.offHour = doc["schedule"]["offHour"] | 0;
  pkt.offMin  = doc["schedule"]["offMin"] | 0;
  pkt.cfgVer  = doc["configVersion"] | 1;
  pkt.regIntervalMs = doc["intervals"]["register"] | 600000;     // fallback 10 min
  pkt.statusIntervalMs = doc["intervals"]["status"] | 60000;   // fallback 1 min


  // send over LoRa (binary)
  sendLoRaPacket((uint8_t*)&pkt, sizeof(pkt));

  Serial.printf("[GATEWAY] Forwarded config to node %s (from topic %s)\n", pkt.nodeId, topic);
}

void handleDeviceConfig(const JsonDocument& doc, const char* topic) {
  Serial.println("[MQTT] Received bootstrap config");

  String gw = String(doc["gatewayId"] | "");
  if (gw == "") {
    Serial.println("[MQTT] Missing gatewayId in bootstrap config, ignoring");
    return;
  }

  if (!saveConfigToSPIFFS(doc)) {
    Serial.println("[BOOTSTRAP] Failed to save config");
    return;
  }

  if (!loadConfigFromSPIFFS()) {
    Serial.println("[BOOTSTRAP] Failed to load config after save");
    return;
  }

  // Re-subscribe and re-init LoRa with new params
  mqtt.subscribe(topic_gateway_config_set.c_str());
  mqtt.subscribe(topic_gateway_node_assign.c_str());
  mqtt.subscribe(topic_gateway_node_config.c_str());
  mqtt.subscribe(topic_gateway_control.c_str());

  // Reinitialize LoRa with new config so node comms pick up immediately
  applyLoRaParamsAndStart();

  // publish ONLINE
  StaticJsonDocument<256> resp;
  resp["type"] = "status";
  resp["status"] = "ONLINE";
  resp["gatewayId"] = GATEWAY_ID;
  String s; serializeJson(resp, s);
  mqtt.publish(topic_gateway_status.c_str(), s.c_str(), true);

  Serial.printf("[BOOTSTRAP] Config applied successfully for gateway %s\n", GATEWAY_ID.c_str());
}

void controlNode(const JsonDocument& doc) {
  const char* nodeId = doc["nodeId"] | "";
  const char* gwId   = doc["gatewayId"] | "";
  const char* action = doc["action"] | "";
  const char* mode   = doc["mode"] | "MANUAL";  // default MANUAL if missing

  if (!nodeId[0] || !gwId[0] || !action[0]) {
    Serial.println("[GATEWAY] Invalid control payload");
    return;
  }

  ControlPkt pkt;
  pkt.pktType = 0x07;
  strncpy(pkt.nodeId, nodeId, sizeof(pkt.nodeId) - 1);
  pkt.nodeId[sizeof(pkt.nodeId) - 1] = '\0';
  strncpy(pkt.gatewayId, gwId, sizeof(pkt.gatewayId) - 1);
  pkt.gatewayId[sizeof(pkt.gatewayId) - 1] = '\0';

  if (strcasecmp(mode, "AUTO") == 0 || strcasecmp(action, "AUTO") == 0) {
    pkt.lightOn = false; // node interprets AUTO mode separately
    Serial.printf("[GATEWAY] AUTO control sent to %s\n", nodeId);
  } else {
    pkt.lightOn = strcasecmp(action, "ON") == 0;
    Serial.printf("[GATEWAY] MANUAL control -> Node %s [%s]\n", nodeId, pkt.lightOn ? "ON" : "OFF");
  }
  enqueuePendingCommand(doc);
}



void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.print("[MQTT] Invalid JSON received: ");
    Serial.println(err.c_str());
    return;
  }

  const char* type = doc["type"] | "";
  if (!type) {
    Serial.println("[MQTT] Missing type field");
    return;
  }

  if (strcmp(type, "node_config") == 0) {
    handleNodeConfig(doc, topic);
  } else if (strcmp(type, "device_config") == 0) {
    handleDeviceConfig(doc, topic);
  } else if (strcmp(type, "gateway_reboot") == 0) {
    // optional: implement
  } else if (strcmp(type, "node_control") == 0) {
    Serial.println("[MQTT] Node control message received");
    controlNode(doc);
}
  else {
    Serial.printf("[MQTT] Unknown message type: %s\n", type);
  }
}

// ---------------- MQTT connect ----------------
bool mqttConnect() {
  String clientId = "Gateway-" + deviceIdStr;
  String lwtTopic;
  if (GATEWAY_ID.length() > 0) lwtTopic = topic_gateway_status;
  else lwtTopic = topic_device_register;

  const char *lwtTopicC = lwtTopic.c_str();
  if (mqtt.connect(clientId.c_str(), NULL, NULL, lwtTopicC, 1, true, "OFFLINE")) {
    Serial.println("[MQTT] Connected to broker");
    digitalWrite(LED_CONN, HIGH);

    // subscribe initial device-level topics so backend can reply before gatewayId assigned
    mqtt.subscribe(topic_device_config_set.c_str());
    mqtt.subscribe(topic_device_register.c_str());

    // subscribe gateway topics if already configured
    if (GATEWAY_ID.length() > 0) {
      mqtt.subscribe(topic_gateway_config_set.c_str());
      mqtt.subscribe(topic_gateway_node_assign.c_str());
      mqtt.subscribe(topic_gateway_node_config.c_str());
      mqtt.subscribe(topic_gateway_control.c_str());
      mqtt.subscribe(topic_node_control.c_str());

      StaticJsonDocument<256> doc;
      doc["type"] = "status";
      doc["status"] = "ONLINE";
      doc["gatewayId"] = GATEWAY_ID;
      doc["nodeCount"] = (int)nodeCount;
      String s; serializeJson(doc, s);
      mqtt.publish(topic_gateway_status.c_str(), s.c_str(), true);
    } else {
      // device-level register to notify backend we are here (bootstrap)
      StaticJsonDocument<256> doc;
      doc["type"] = "device_register";
      doc["deviceId"] = deviceIdStr;
      doc["firmwareVersion"] = "1.0.0";
      String s; serializeJson(doc, s);
      mqtt.publish(topic_generic_register.c_str(), s.c_str());
      mqtt.publish(topic_device_register.c_str(), s.c_str());
    }
    return true;
  } else {
    Serial.println("[MQTT] connect failed");
    digitalWrite(LED_CONN, LOW);
    return false;
  }
}

// ---------------- Publish node registration (forward from LoRa node to backend) ----------------
void publishNodeRegister(const char* nodeId, int rssi, float snr) {
  StaticJsonDocument<256> doc;
  doc["type"] = "node_register";
  doc["deviceId"] = deviceIdStr;
  doc["gatewayId"] = GATEWAY_ID;
  doc["nodeId"] = nodeId;
  doc["rssi"] = rssi;
  doc["snr"] = snr;
  doc["timestamp"] = millis();
  String s; serializeJson(doc, s);

  String topic;
  if (GATEWAY_ID.length() > 0) topic = "iot/gateway/" + GATEWAY_ID + "/node/" + String(nodeId) + "/register";
  else topic = "iot/gateway/" + deviceIdStr + "/node/" + String(nodeId) + "/register";

  mqtt.publish(topic.c_str(), s.c_str());
}

// ---------------- LoRa receive handling ----------------
void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;
  uint8_t pktType = LoRa.read();

  Serial.printf("[LORA_RECEIVE] PktType=%02X packetSize=%d\n", pktType, packetSize);

  if (pktType == 0x02) { // RegisterPkt from a node
    RegisterPkt reg;
    reg.pktType = 0x02;
    LoRa.readBytes(((uint8_t*)&reg) + 1, sizeof(reg)-1);
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    if (mqtt.connected()) publishNodeRegister(reg.nodeId, rssi, snr);
    Serial.printf("[LORA] Node register from %s rssi=%d snr=%.1f\n", reg.nodeId, rssi, snr);

  } else if (pktType == 0x05) { // STATUS

    const size_t expected = 1 + sizeof(PolePacket);  // 59
    if (packetSize != expected) {
      Serial.printf("[LORA] Bad packet size: %d, expected %d\n", packetSize, expected);
      while (LoRa.available()) LoRa.read();
      return;
    }
    uint8_t buffer[expected];
    int bytesRead = LoRa.readBytes(buffer, expected);
    if (bytesRead != expected) {
      Serial.println("[LORA] Incomplete status packet");
      while (LoRa.available()) LoRa.read();
      return;
    }

    PolePacket* pkt = (PolePacket*)(buffer + 1);

    // Now publish
    StaticJsonDocument<256> doc;
    doc["type"] = "node_status";
    doc["deviceId"] = deviceIdStr;
    doc["gatewayId"] = GATEWAY_ID;
    doc["nodeId"] = pkt->nodeId;
    doc["state"] = pkt->lightState ? "ON" : "OFF";
    doc["fault"] = pkt->fault;
    doc["time"] = String(pkt->hour) + ":" + String(pkt->minute);
    doc["rssi"] = pkt->rssi;
    doc["snr"] = pkt->snr;
    String s; serializeJson(doc, s);

    String topic = "iot/gateway/" + GATEWAY_ID + "/node/" + String(pkt->nodeId) + "/status";
    mqtt.publish(topic.c_str(), s.c_str());
    blinkDataLED();
   
  } else if (pktType == 0x06) { // ACK
    // AckPkt ack;
    // ack.pktType = pktType;
    // LoRa.readBytes(((uint8_t*)&ack)+1, sizeof(ack)-1);
    // StaticJsonDocument<128> doc;
    // doc["type"] = "node_config_ack";
    // doc["nodeId"] = ack.nodeId;
    // doc["gatewayId"] = GATEWAY_ID;
    // doc["cfgVer"] = ack.cfgVer;
    // String s; serializeJson(doc, s);
    // if (GATEWAY_ID.length() > 0) {
    //   String topic = "iot/gateway/" + GATEWAY_ID + "/node/" + String(ack.nodeId) + "/config/ack";
    //   mqtt.publish(topic.c_str(), s.c_str(), true);
    // }
    // Serial.printf("[LORA] Config ACK from %s cfgVer=%d\n", ack.nodeId, ack.cfgVer);
    AckPkt ack;
    ack.pktType = pktType;
    LoRa.readBytes(((uint8_t*)&ack)+1, sizeof(ack)-1);

    if (currentCmdIndex >= 0) {
        PendingCommand &c = cmdQueue[currentCmdIndex];

        if (strcmp(c.nodeId, ack.nodeId) == 0) {

            Serial.printf("[CMD] ACK received for %s\n", c.nodeId);

            c.active = false;
            c.waitingAck = false;
            currentCmdIndex = -1;
        }
    }
  } else {
    // Unknown/other packets: flush
    while (LoRa.available()) LoRa.read();
  }
}

// ---------------- Broadcast beacon over LoRa ----------------
void broadcastBeacon() {
  BeaconPkt b;
  b.pktType = 0x01;
  b.uptime_s = (uint32_t)(millis() / 1000);
  sendLoRaPacket((uint8_t*)&b, sizeof(b), true);
}

// ---------------- Setup & Loop ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Gateway Starting ===");
  Serial.printf("PolePacket size: %d\n", sizeof(PolePacket));

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_CONN, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_CONN, LOW);
  digitalWrite(LED_DATA, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] init failed");
  } else {
    Serial.println("[SPIFFS] mounted successfully");
  }

  deviceIdStr = getDeviceId();
  Serial.printf("[BOOT] DeviceId=%s\n", deviceIdStr.c_str());

  backendDeviceTopicBase = "iot/gateway/" + deviceIdStr + "/";
  topic_device_config_set = backendDeviceTopicBase + "config/set";
  topic_device_register = backendDeviceTopicBase + "register";

  Serial.println("[BOOT] Loading config (if exists)...");
  bool ok = loadConfigFromSPIFFS();
  if (ok) Serial.println("[CONFIG] Existing configuration loaded");
  else Serial.println("[CONFIG] No config file found; entering bootstrap mode");

  // LoRa init with defaults or loaded values
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  applyLoRaParamsAndStart();

  // SIM900 / GPRS init (best-effort)
  sim900.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  modem.restart();
  if (modem.gprsConnect(APN.c_str(), "", "")) {
    Serial.println("[SIM900A] GPRS Connected");
  } else {
    Serial.println("[SIM900A] GPRS Failed (continuing)");
  }

  mqtt.setServer(MQTT_BROKER.c_str(), MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  Serial.println("[BOOT] Setup complete.");
}

void loop() {
  // === GPRS: Check every 1s, auto-reconnect ===
  if (millis() - lastGprsCheck >= GPRS_CHECK_INTERVAL) {
    lastGprsCheck = millis();

    bool isConnected = modem.isNetworkConnected() && modem.isGprsConnected();

    if (isConnected) {
      if (!gprsWasConnected) {
        Serial.println("[SIM900A] GPRS recovered!");
        digitalWrite(LED_CONN, HIGH);
      }
      gprsWasConnected = true;
      gprsFailStart = 0;
      lastGprsAttempt = 0;
    } else {
      if (gprsWasConnected) {
        Serial.println("[SIM900A] GPRS lost! Starting recovery...");
        digitalWrite(LED_CONN, LOW);
      }
      gprsWasConnected = false;

      if (lastGprsAttempt == 0 || (millis() - lastGprsAttempt >= GPRS_RECONNECT_DELAY)) {
        lastGprsAttempt = millis();
        connectGPRS();
      }
    }
  }

  // === MQTT: Only try if GPRS is up ===
  if (modem.isGprsConnected()) {
    if (!mqtt.connected()) {
      if (millis() - lastMqttReconnect > 3000) {
        mqtt.setServer(MQTT_BROKER.c_str(), MQTT_PORT);
        if (mqttConnect()) {
          lastMqttReconnect = 0;
        } else {
          lastMqttReconnect = millis();
        }
      }
    } else {
      mqtt.loop();
    }
  } else {
    // Optional: force MQTT disconnect if GPRS is down
    if (mqtt.connected()) {
      mqtt.disconnect();
    }
  }

  handleLoRaReceive();
  processPendingCommands();


  if (millis() - lastBeacon >= BEACON_INTERVAL) {
    lastBeacon = millis();
    broadcastBeacon();
  }

  if (millis() - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = millis();
    StaticJsonDocument<256> doc;
    doc["type"] = "telemetry";
    doc["deviceId"] = deviceIdStr;
    doc["gatewayId"] = GATEWAY_ID;
    doc["uptime_s"] = millis() / 1000;
    doc["nodeCount"] = (int)nodeCount;
    String s; serializeJson(doc, s);
    if (mqtt.connected()) {
      if (GATEWAY_ID.length() > 0) mqtt.publish(topic_gateway_status.c_str(), s.c_str(), true);
      else mqtt.publish((String("iot/gateway/") + deviceIdStr + "/status").c_str(), s.c_str(), true);
    }
  }

  delay(10);
}
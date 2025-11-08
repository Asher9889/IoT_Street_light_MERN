/* Gateway Firmware - backend-driven dynamic config
   Optimized implementation with bootstrap + server-authoritative control
   Requires: ArduinoJson, SPIFFS/LittleFS, LoRa, TinyGsm, PubSubClient
   Notes:
     - Uses deviceId (ESP.getEfuseMac()) for initial handshake
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
#include <WiFi.h> // used only for obtaining MAC if needed

// ---------------- LoRa Pins (adjust to your board) ----------------
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

// ---------------- GSM Pins ----------------
#define MODEM_RX 16
#define MODEM_TX 17
#define DEFAULT_APN "airtelgprs.com"

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
unsigned long gprsFailStart = 0;
const unsigned long GPRS_FAIL_TIMEOUT = 5 * 60 * 1000UL; // 5 minutes

// ---------------- Config structures ----------------
struct NodeInfo {
  uint8_t nodeId;
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
uint32_t LORA_FREQUENCY = 433000000UL;
String APN = DEFAULT_APN;
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
String topic_gateway_node_config; // iot/gateway/<gatewayId>/node/config
String topic_gateway_status;      // iot/gateway/<gatewayId>/status
String topic_gateway_control;     // iot/gateway/<gatewayId>/control
String topic_generic_register = "iot/gateway/register"; // global backend listen

// ---------------- Packed structs used over LoRa (compact binary packets) ----------------
struct __attribute__((packed)) BeaconPkt {
  uint8_t pktType; // 0x01
  // gatewayId not sent here (not known to LoRa-only nodes) but kept for completeness
  uint32_t uptime_s;
};
struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType; // 0x02
  char nodeId[24];            // 16 chars + null
  uint8_t fwVersion;
  uint32_t uptime_s;
};
struct __attribute__((packed)) AssignPkt {
  uint8_t pktType; // 0x03
  char nodeId[24];            // 16 chars + null
  // gatewayId may be implied; using numeric small id not used here because nodes use simple local nodeId
};
struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType;            // 0x04
  char nodeId[24];            // 16 chars + null
  char gatewayId[24];         // 16 chars + null (optional)
  uint8_t onHour;
  uint8_t onMin;
  uint8_t offHour;
  uint8_t offMin;
  uint8_t cfgVer;
};
struct __attribute__((packed)) PolePacket {
  char nodeId[24];            // 16 chars + null
  char gatewayId[24];         // 16 chars + null (optional)
  bool lightState;
  bool fault;
  uint8_t hour;
  uint8_t minute;
  int rssi;
  int snr;
};

// ---------------- Helpers ----------------
void blinkDataLED(int duration = 50) {
  digitalWrite(LED_DATA, HIGH);
  delay(duration);
  digitalWrite(LED_DATA, LOW);
}

String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, 13, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  return String("device") + id;
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
  APN = String(doc["apn"] | APN);
  MQTT_BROKER = String(doc["mqtt"]["broker"] | MQTT_BROKER);
  MQTT_PORT = doc["mqtt"]["port"] | MQTT_PORT;
  configVersion = doc["configVersion"] | configVersion;

  // load nodes array
  JsonArray nodes = doc["nodes"].as<JsonArray>();
  nodeCount = 0;
  if (!nodes.isNull()) {
    for (JsonObject n : nodes) {
      if (nodeCount >= MAX_NODES) break;
      nodeList[nodeCount].nodeId = n["nodeId"] | 0;
      nodeList[nodeCount].onHour = n["config"]["onHour"] | 0;
      nodeList[nodeCount].onMin = n["config"]["onMin"] | 0;
      nodeList[nodeCount].offHour = n["config"]["offHour"] | 0;
      nodeList[nodeCount].offMin = n["config"]["offMin"] | 0;
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
    topic_gateway_node_config = backendGatewayTopicBase + "node/config";
    topic_gateway_status = backendGatewayTopicBase + "status";
    topic_gateway_control = backendGatewayTopicBase + "control";
  }
  Serial.printf("[CONFIG] loaded gatewayId=%s nodes=%d freq=%lu broker=%s:%d\n",
                GATEWAY_ID.c_str(), nodeCount, LORA_FREQUENCY, MQTT_BROKER.c_str(), MQTT_PORT);
  return true;
}

// ---------------- MQTT callback ----------------
// void onMqttMessage(char* topic, byte* payload, unsigned int length) {
//   String t = String(topic);
//   String msg;
//   msg.reserve(length + 1);
//   for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

//   Serial.printf("[MQTT] %s -> %s\n", t.c_str(), msg.c_str());

//   // 1) Device-level config (during bootstrap)
//   if (t == topic_device_config_set) {
//     // Backend sent config for deviceId (bootstrap reply)
//     StaticJsonDocument<4096> doc;
//     DeserializationError err = deserializeJson(doc, msg);
//     if (err) {
//       Serial.println("[MQTT] invalid device config JSON");
//       return;
//     }
//     // Expect backend to include gatewayId in the payload. If not, backend can return partial config; handle defensively.
//     String gw = String(doc["gatewayId"] | "");
//     if (gw == "") {
//       Serial.println("[MQTT] device config missing gatewayId, ignoring");
//       return;
//     }
//     // Save and apply
//     if (saveConfigToSPIFFS(doc)) {
//       loadConfigFromSPIFFS();
//       // after saving, switch to gateway topic base; confirm online
//       mqtt.publish(topic_gateway_status.c_str(), "{\"type\":\"status\",\"status\":\"ONLINE\"}", true);
//       Serial.println("[MQTT] Bootstrap config applied and persisted.");
//     }
//     return;
//   }

//   // 2) Gateway-level config update
//   if (GATEWAY_ID.length() > 0 && t == topic_gateway_config_set) {
//     StaticJsonDocument<4096> doc;
//     DeserializationError err = deserializeJson(doc, msg);
//     if (err) {
//       Serial.println("[MQTT] invalid gateway config JSON");
//       return;
//     }
//     int newVersion = doc["configVersion"] | 0;
//     if (newVersion <= configVersion) {
//       Serial.println("[MQTT] Received config with older or same version; skipping");
//       return;
//     }
//     if (saveConfigToSPIFFS(doc)) {
//       if (loadConfigFromSPIFFS()) {
//         configVersion = newVersion;
//       }
//     }
//     return;
//   }

//   // 3) Assignment messages - backend instructs gateway to assign a node locally (over LoRa)
//   if (GATEWAY_ID.length() > 0 && t == topic_gateway_node_assign) {
//     StaticJsonDocument<256> doc;
//     DeserializationError err = deserializeJson(doc, msg);
//     if (err) {
//       Serial.println("[MQTT] invalid assignment JSON");
//       return;
//     }
//     const char* type = doc["type"] | "";
//     if (strcmp(type, "assign_node") == 0) {
//       uint8_t nodeId = doc["nodeId"] | 0;
//       // send ASSIGN packet over LoRa (AssignPkt)
//       AssignPkt ap;
//       ap.pktType = 0x03;
//       ap.nodeId = nodeId;
//       LoRa.beginPacket();
//       LoRa.write((uint8_t*)&ap, sizeof(ap));
//       LoRa.endPacket();
//       blinkDataLED(100);
//     }
//     return;
//   }

//   // 4) Node-specific config sent by backend to gateway (gateway should forward to node)
//   if (GATEWAY_ID.length() > 0 && t == topic_gateway_node_config) {
//     StaticJsonDocument<256> doc;
//     DeserializationError err = deserializeJson(doc, msg);
//     if (err) {
//       Serial.println("[MQTT] invalid node config JSON");
//       return;
//     }
//     uint8_t nodeId = doc["nodeId"] | 0;
//     uint8_t onHour = doc["config"]["onHour"] | 0;
//     uint8_t onMin = doc["config"]["onMin"] | 0;
//     uint8_t offHour = doc["config"]["offHour"] | 0;
//     uint8_t offMin = doc["config"]["offMin"] | 0;
//     uint8_t cfgVer = doc["configVersion"] | 0;
//     // pack and send LoRa ConfigPkt
//     ConfigPkt cp;
//     cp.pktType = 0x04;
//     cp.nodeId = nodeId;
//     cp.onHour = onHour; cp.onMin = onMin;
//     cp.offHour = offHour; cp.offMin = offMin;
//     cp.cfgVer = cfgVer;
//     LoRa.beginPacket();
//     LoRa.write((uint8_t*)&cp, sizeof(cp));
//     LoRa.endPacket();
//     blinkDataLED(100);
//     return;
//   }

//   // 5) Control commands (reboot, reset, ota)
//   if (GATEWAY_ID.length() > 0 && t == topic_gateway_control) {
//     StaticJsonDocument<256> doc;
//     DeserializationError err = deserializeJson(doc, msg);
//     if (err) {
//       Serial.println("[MQTT] invalid control JSON");
//       return;
//     }
//     const char* cmd = doc["cmd"] | "";
//     if (strcmp(cmd, "reboot") == 0) {
//       mqtt.publish(topic_gateway_status.c_str(), "{\"type\":\"status\",\"status\":\"REBOOTING\"}", true);
//       delay(500);
//       ESP.restart();
//     } else if (strcmp(cmd, "reset_config") == 0) {
//       if (SPIFFS.exists(CONFIG_PATH)) SPIFFS.remove(CONFIG_PATH);
//       mqtt.publish(topic_gateway_status.c_str(), "{\"type\":\"status\",\"status\":\"CONFIG_RESET\"}", true);
//     } else if (strcmp(cmd, "reboot_radio") == 0) {
//       LoRa.end();
//       delay(200);
//       LoRa.begin(LORA_FREQUENCY);
//       mqtt.publish(topic_gateway_status.c_str(), "{\"type\":\"status\",\"status\":\"RADIO_REBOOTED\"}", true);
//     }
//     return;
//   }

//   // otherwise ignore unknown topics
//   Serial.println("[MQTT] Unhandled topic");
// }

// helper: extract component between tokens, e.g. topic = "iot/gateway/GW1/node/node123/config/set"
// returns node id (token index 4, zero-based) -> "node123"
String extractnodeIdFromTopic(const char* topic) {
  // split by '/'
  String t = String(topic);
  int parts = 0;
  int start = 0;
  for (int i = 0; i < t.length(); ++i) {
    if (t[i] == '/') {
      // token from start..i-1
      ++parts;
      if (parts == 4) { // 0:iot 1:gateway 2:<gatewayId> 3:node 4:<nodeId>
        int nextSlash = t.indexOf('/', i+1);
        if (nextSlash == -1) nextSlash = t.length();
        return t.substring(i+1, nextSlash);
      }
    }
  }
  return String("");
}

void handleNodeConfig(const JsonDocument& doc, const char* topic) {
  ConfigPkt pkt;
  pkt.pktType = 0x04;

  // get nodeId from payload (preferred)
  const char* nodeIdPayload = doc["nodeId"] | "";
  String nodeIdStr = nodeIdPayload;
  if (nodeIdStr.length() == 0) {
    // fallback: extract from topic (since we subscribed to node/+/config/set)
    nodeIdStr = extractnodeIdFromTopic(topic);
  }
  // safe copy - ensure null termination
  strncpy(pkt.nodeId, nodeIdStr.c_str(), sizeof(pkt.nodeId)-1);
  pkt.nodeId[sizeof(pkt.nodeId)-1] = '\0';

  // gatewayId may be present
  const char* gw = doc["gatewayId"] | "";
  strncpy(pkt.gatewayId, gw, sizeof(pkt.gatewayId)-1);
  pkt.gatewayId[sizeof(pkt.gatewayId)-1] = '\0';

  // schedule
  pkt.onHour = doc["schedule"]["onHour"] | 0;
  pkt.onMin  = doc["schedule"]["onMin"]  | 0;
  pkt.offHour = doc["schedule"]["offHour"] | 0;
  pkt.offMin  = doc["schedule"]["offMin"] | 0;
  pkt.cfgVer  = doc["configVersion"] | 1;

  // send over LoRa (binary)
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&pkt, sizeof(pkt));
  LoRa.endPacket();

  Serial.printf("[GATEWAY] Forwarded config to node %s (from topic %s)\n", pkt.nodeId, topic);
}


void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload, length)) {
    Serial.println("[MQTT] Invalid JSON received");
    return;
  }

  const char* type = doc["type"] | "";

  if (!type) {
    Serial.println("[MQTT] Missing type field");
    return;
  }

  if (strcmp(type, "node_config") == 0) {
    handleNodeConfig(doc, topic);
  } 
  else if (strcmp(type, "firmware_update") == 0) {
    // handleFirmwareUpdate(doc, topic);
  } 
  else if (strcmp(type, "gateway_reboot") == 0) {
    // handleGatewayReboot(doc);
  } 
  else {
    Serial.printf("[MQTT] Unknown message type: %s\n", type);
  }
}




// ---------------- MQTT connect (supports bootstrap by deviceId if gatewayId not present) ----------------
bool mqttConnect() {
  // build clientId: use deviceId to ensure uniqueness across devices on broker
  String clientId = "Gateway-" + deviceIdStr;
  // choose LWT: use gateway-level status topic if available, else device-level
  String lwtTopic;
  if (GATEWAY_ID.length() > 0) lwtTopic = topic_gateway_status;
  else lwtTopic = topic_device_register; // device-level register status

  const char *lwtTopicC = lwtTopic.c_str();
  if (mqtt.connect(clientId.c_str(), NULL, NULL, lwtTopicC, 1, true, "OFFLINE")) {
    Serial.println("[MQTT] Connected to broker");
    digitalWrite(LED_CONN, HIGH);

    // subscribe initial device-level topics so backend can reply before gatewayId assigned
    mqtt.subscribe(topic_device_config_set.c_str());
    mqtt.subscribe(topic_device_register.c_str()); // may not be needed but ok

    // if gateway already configured, subscribe gateway-level topics too
    if (GATEWAY_ID.length() > 0) {
      mqtt.subscribe(topic_gateway_config_set.c_str());
      mqtt.subscribe(topic_gateway_node_assign.c_str());
      mqtt.subscribe(topic_gateway_node_config.c_str());
      mqtt.subscribe(topic_gateway_control.c_str());
      // publish ONLINE
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
      // Also publish to device register topic
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
void publishNodeRegister(char* nodeId, int rssi, float snr) {
  StaticJsonDocument<256> doc;
  doc["type"] = "node_register";
  doc["deviceId"] = deviceIdStr;
  doc["gatewayId"] = GATEWAY_ID;
  doc["nodeId"] = nodeId;
  doc["rssi"] = rssi;
  doc["snr"] = snr;
  doc["timestamp"] = millis();
  String s; serializeJson(doc, s);

  // publish to gateway node register path
  if (GATEWAY_ID.length() > 0) {
    String topic = String("iot/gateway/") + GATEWAY_ID + "/node/register";
    mqtt.publish(topic.c_str(), s.c_str());
  } else {
    // publish to device-level (backend must handle)
    String topic = String("iot/gateway/") + deviceIdStr + "/node/register";
    mqtt.publish(topic.c_str(), s.c_str());
  }

}

// ---------------- LoRa receive handling ----------------
void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  uint8_t pktType = 0;
  LoRa.readBytes(&pktType, 1);

  if (pktType == 0x02) { // RegisterPkt from a node
    RegisterPkt reg;
    reg.pktType = 0x02;
    // we've read the first byte already
    LoRa.readBytes(((uint8_t*)&reg) + 1, sizeof(reg)-1);
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    // forward to backend
    if (mqtt.connected()) publishNodeRegister(reg.nodeId, rssi, snr);
  } else if (pktType == 0x05) { // STATUS (wrapped PolePacket)
    PolePacket pkt;
    // here we assume full PolePacket follows
    LoRa.readBytes((uint8_t*)&pkt, sizeof(pkt));
                  pkt.nodeId, pkt.lightState ? "ON" : "OFF",
                  pkt.fault ? "YES" : "NO",
                  pkt.hour, pkt.minute,
                  pkt.rssi, pkt.snr;

    // build JSON and publish
    StaticJsonDocument<256> doc;
    doc["type"] = "node_status";
    doc["deviceId"] = deviceIdStr;
    doc["gatewayId"] = GATEWAY_ID;
    doc["nodeId"] = pkt.nodeId;
    doc["state"] = pkt.lightState ? "ON":"OFF";
    doc["fault"] = pkt.fault;
    doc["time"] = String(pkt.hour) + ":" + String(pkt.minute);
    doc["rssi"] = pkt.rssi;
    doc["snr"] = pkt.snr;
    String s; serializeJson(doc, s);

    String topic;
    if (GATEWAY_ID.length() > 0) topic = String("iot/gateway/") + GATEWAY_ID + "/node/" + String(pkt.nodeId) + "/status";
    else topic = String("iot/gateway/") + deviceIdStr + "/node/" + String(pkt.nodeId) + "/status";

    mqtt.publish(topic.c_str(), s.c_str());
    blinkDataLED();
  } else {
    // Unknown packet type — flush and ignore
    while (LoRa.available()) LoRa.read();
  }
}

// ---------------- Broadcast beacon over LoRa (optional, useful for discovery) ----------------
void broadcastBeacon() {
  BeaconPkt b;
  b.pktType = 0x01;
  b.uptime_s = (uint32_t)(millis() / 1000);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&b, sizeof(b));
  LoRa.endPacket();
  blinkDataLED(10);
}

// ---------------- Setup ----------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Gateway Starting ===");
  Serial.println("[BOOT] Initializing hardware...");

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_CONN, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_CONN, LOW);
  digitalWrite(LED_DATA, LOW);

  // --- LOG ---

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] init failed");
  } else {
    Serial.println("[SPIFFS] mounted successfully");
  }

  deviceIdStr = getDeviceId();
  Serial.println("===========================================");
  Serial.print("[DEBUG] Device ID (from efuse): ");
  Serial.println(deviceIdStr);
  Serial.println("===========================================");
  Serial.printf("[BOOT] DeviceId=%s\n", deviceIdStr.c_str());

  backendDeviceTopicBase = "iot/gateway/" + deviceIdStr + "/";
  topic_device_config_set = backendDeviceTopicBase + "config/set";
  topic_device_register = backendDeviceTopicBase + "register";

  // --- LOG ---
  Serial.println("[BOOT] Loading config (if exists)...");
  bool ok = loadConfigFromSPIFFS();
  if (ok) Serial.println("[CONFIG] Existing configuration loaded");
  else Serial.println("[CONFIG] No config file found; entering bootstrap mode");

  // --- LOG ---
  Serial.println("[BOOT] Initializing LoRa...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[LoRa] init FAILED");
    while (1) delay(1000);
  }

  // --- LOG ---
  Serial.println("[BOOT] Initializing SIM900A modem...");
  sim900.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  modem.restart();
  if (modem.gprsConnect(APN.c_str(), "", "")) {
  } else {
    Serial.println("[SIM900A] GPRS Failed");
  }

  // --- LOG ---
  mqtt.setServer(MQTT_BROKER.c_str(), MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  Serial.println("[BOOT] Setup complete.");
}

void loop() {
  // --- LOG ---
  if (!modem.isGprsConnected()) {
    Serial.println("[SIM900A] GPRS disconnected...");
    if (gprsFailStart == 0) gprsFailStart = millis();
    if (millis() - gprsFailStart > GPRS_FAIL_TIMEOUT) {
      Serial.println("[SIM900A] GPRS timeout, restarting device...");
      ESP.restart();
    }
    Serial.println("[SIM900A] Attempting GPRS reconnect...");
    if (modem.gprsConnect(APN.c_str(), "", "")) {
      Serial.println("[SIM900A] GPRS Reconnected");
      gprsFailStart = 0;
    } else {
      delay(2000);
    }
  } else {
    gprsFailStart = 0;
  }

  if (!mqtt.connected()) {
    if (millis() - lastMqttReconnect > 3000) {
      Serial.println("[MQTT] Attempting reconnect...");
      mqtt.setServer(MQTT_BROKER.c_str(), MQTT_PORT);
      if (mqttConnect()) {
        Serial.println("[MQTT] Reconnected successfully");
        lastMqttReconnect = 0;
      } else {
        Serial.println("[MQTT] Reconnect failed");
        lastMqttReconnect = millis();
      }
    }
  } else {
    mqtt.loop();
  }

  handleLoRaReceive();

  if (millis() - lastBeacon >= BEACON_INTERVAL) {
    lastBeacon = millis();
    Serial.println("[LORA] Sending beacon...");
    broadcastBeacon();
  }

  if (millis() - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = millis();
    Serial.println("[MQTT] Publishing telemetry...");
    StaticJsonDocument<256> doc;
    doc["type"] = "telemetry";
    doc["deviceId"] = deviceIdStr;
    doc["gatewayId"] = GATEWAY_ID;
    doc["uptime_s"] = millis() / 1000;
    doc["nodeCount"] = (int)nodeCount;
    String s; serializeJson(doc, s);
    if (mqtt.connected()) {
      if (GATEWAY_ID.length() > 0)
        mqtt.publish(topic_gateway_status.c_str(), s.c_str(), true);
      else
        mqtt.publish((String("iot/gateway/") + deviceIdStr + "/status").c_str(), s.c_str(), true);
      Serial.println("[MQTT] Telemetry published successfully");
    } else {
      Serial.println("[MQTT] Telemetry publish skipped (not connected)");
    }
  }

  delay(10);
}

/* Gateway Firmware - backend-driven dynamic config
   - Save as Gateway.ino
   - Requires: ArduinoJson, SPIFFS/LittleFS, LoRa, TinyGsm, PubSubClient
*/

#include <SPI.h>
#include <LoRa.h>
#include <FS.h>
#include <SPIFFS.h>
#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <Preferences.h>

// ---------------- LoRa Pins ----------------
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

HardwareSerial sim900(1);

// ---------------- MQTT ----------------
// Default/fallback MQTT broker (backend can push override in config)
const char* DEFAULT_BROKER = "103.20.215.109";
const int DEFAULT_PORT = 1883;

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

// ---------------- Config ----------------
struct NodeInfo {
  uint8_t nodeId;
  uint8_t onHour;
  uint8_t onMin;
  uint8_t offHour;
  uint8_t offMin;
  uint8_t configVersion;
};

#define MAX_NODES 64
NodeInfo nodeList[MAX_NODES];
size_t nodeCount = 0;

uint8_t GATEWAY_ID = 0; // loaded from config
uint32_t LORA_FREQUENCY = 433000000UL;
String APN = DEFAULT_APN;
String MQTT_BROKER = DEFAULT_BROKER;
int MQTT_PORT = DEFAULT_PORT;
String backendConfigTopic;       // backend/config/<gatewayId>
String backendAssignmentsTopic;  // backend/assignments/<gatewayId>
String gatewayRegisterTopic;     // gateway/<gatewayId>/register
String gatewayStatusTopic;       // gateway/<gatewayId>/status
int configVersion = 0;

// ---------------- Packed structs ----------------
struct __attribute__((packed)) BeaconPkt {
  uint8_t pktType; // 0x01
  uint8_t gatewayId;
  uint32_t uptime_s;
};
struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType; // 0x02
  uint8_t nodeId;
  uint8_t fwVersion;
  uint32_t uptime_s;
};
struct __attribute__((packed)) AssignPkt {
  uint8_t pktType; // 0x03
  uint8_t nodeId;
  uint8_t gatewayId;
};
struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType; // 0x04
  uint8_t nodeId;
  uint8_t gatewayId;
  uint8_t onHour, onMin;
  uint8_t offHour, offMin;
  uint8_t cfgVer;
};
struct __attribute__((packed)) PolePacket {
  uint8_t nodeID;
  uint8_t gatewayID;
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

// ---------------- Storage ----------------
const char *CONFIG_PATH = "/gateway_config.json";

bool saveConfigToSPIFFS(const JsonDocument &doc) {
  File f = SPIFFS.open(CONFIG_PATH, FILE_WRITE);
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

bool loadConfigFromSPIFFS() {
  if (!SPIFFS.exists(CONFIG_PATH)) return false;
  File f = SPIFFS.open(CONFIG_PATH, FILE_READ);
  if (!f) return false;
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  GATEWAY_ID = doc["gatewayId"] | 0;
  LORA_FREQUENCY = doc["lora"]["frequency"] | 433000000UL;
  APN = String(doc["apn"] | DEFAULT_APN);
  MQTT_BROKER = String(doc["mqtt"]["broker"] | DEFAULT_BROKER);
  MQTT_PORT = doc["mqtt"]["port"] | DEFAULT_PORT;
  configVersion = doc["configVersion"] | 0;

  // load nodes
  JsonArray nodes = doc["nodes"].as<JsonArray>();
  nodeCount = 0;
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

  // update topics
  backendConfigTopic = String("backend/config/") + String(GATEWAY_ID);
  backendAssignmentsTopic = String("backend/assignments/") + String(GATEWAY_ID);
  gatewayRegisterTopic = String("gateway/") + String(GATEWAY_ID) + "/register";
  gatewayStatusTopic = String("gateway/") + String(GATEWAY_ID) + "/status";

  return true;
}

// ---------------- MQTT Callback ----------------
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.printf("[MQTT] %s -> %s\n", topic, msg.c_str());

  // Backend sent full config to gateway
  if (t == backendConfigTopic) {
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, msg);
    if (err) {
      Serial.println("[MQTT] invalid config JSON");
      return;
    }
    saveConfigToSPIFFS(doc); // persist
    loadConfigFromSPIFFS();  // reload into structures
    // apply config: send config packet to nodes
    for (size_t i = 0; i < nodeCount; ++i) {
      ConfigPkt cp;
      cp.pktType = 0x04;
      cp.nodeId = nodeList[i].nodeId;
      cp.gatewayId = GATEWAY_ID;
      cp.onHour = nodeList[i].onHour;
      cp.onMin = nodeList[i].onMin;
      cp.offHour = nodeList[i].offHour;
      cp.offMin = nodeList[i].offMin;
      cp.cfgVer = nodeList[i].configVersion;
      LoRa.beginPacket();
      LoRa.write((uint8_t*)&cp, sizeof(cp));
      LoRa.endPacket();
      delay(50);
      blinkDataLED();
    }
    // ack to backend (optional)
    String ack = "{\"type\":\"config_applied\",\"gatewayId\":";
    ack += String(GATEWAY_ID);
    ack += ",\"configVersion\":";
    ack += String(configVersion);
    ack += "}";
    mqtt.publish(gatewayStatusTopic.c_str(), ack.c_str(), true);
    Serial.println("[MQTT] Applied new config and notified backend.");
    return;
  }

  // Backend sent an assignment message (assign nodeX to this gateway)
  if (t == backendAssignmentsTopic) {
    // expected payload: {"type":"assign_node","nodeId":5}
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);
    if (err) return;
    const char* type = doc["type"];
    if (strcmp(type, "assign_node") == 0) {
      uint8_t nodeId = doc["nodeId"] | 0;
      // send ASSIGN over LoRa
      AssignPkt ap;
      ap.pktType = 0x03;
      ap.nodeId = nodeId;
      ap.gatewayId = GATEWAY_ID;
      LoRa.beginPacket();
      LoRa.write((uint8_t*)&ap, sizeof(ap));
      LoRa.endPacket();
      blinkDataLED(100);
      Serial.printf("[LoRa] Sent ASSIGN to node %d\n", nodeId);
    }
    return;
  }
}

// ---------------- MQTT connect ----------------
bool mqttConnect() {
  if (GATEWAY_ID == 0) return false;
  String clientId = "Gateway-" + String(GATEWAY_ID);
  String lwtTopic = gatewayStatusTopic;
  if (mqtt.connect(clientId.c_str(), NULL, NULL, lwtTopic.c_str(), 1, true, "OFFLINE")) {
    Serial.println("[MQTT] Connected");
    // subscribe to backend topics for this gateway
    mqtt.subscribe(backendConfigTopic.c_str());
    mqtt.subscribe(backendAssignmentsTopic.c_str());
    // publish ONLINE status
    StaticJsonDocument<256> doc;
    doc["type"] = "status";
    doc["status"] = "ONLINE";
    doc["gatewayId"] = GATEWAY_ID;
    doc["nodeCount"] = (int)nodeCount;
    String s; serializeJson(doc, s);
    mqtt.publish(gatewayStatusTopic.c_str(), s.c_str(), true);
    digitalWrite(LED_CONN, HIGH);
    return true;
  } else {
    Serial.println("[MQTT] connect failed");
    digitalWrite(LED_CONN, LOW);
    return false;
  }
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Gateway Starting ===");

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_CONN, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_CONN, LOW);
  digitalWrite(LED_DATA, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] init failed");
  }

  // try load config from SPIFFS
  bool ok = loadConfigFromSPIFFS();
  if (!ok) {
    Serial.println("[CONFIG] no local config, waiting for backend config.");
    // We still need a gatewayId to subscribe; default to 0 and wait for MQTT? We'll attempt to connect with default broker only.
  } else {
    Serial.printf("[CONFIG] loaded gatewayId=%d nodes=%d freq=%lu\n", GATEWAY_ID, nodeCount, LORA_FREQUENCY);
  }

  // LoRa Init
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[LoRa] init FAILED");
    while (1) delay(1000);
  }
  Serial.println("[LoRa] Init OK");

  // GSM Init
  sim900.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  Serial.println("[SIM900A] Starting...");
  modem.restart();
  if (modem.gprsConnect(APN.c_str(), "", "")) Serial.println("[SIM900A] GPRS Connected");
  else Serial.println("[SIM900A] GPRS Failed");

  // MQTT Init
  mqtt.setServer(MQTT_BROKER.c_str(), MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
}

// ---------------- Publish node registration to backend ----------------
void publishNodeRegister(uint8_t nodeId, int rssi, float snr) {
  StaticJsonDocument<256> doc;
  doc["type"] = "node_register";
  doc["gatewayId"] = GATEWAY_ID;
  doc["nodeId"] = nodeId;
  doc["rssi"] = rssi;
  doc["snr"] = snr;
  doc["timestamp"] = millis();
  String s; serializeJson(doc, s);
  mqtt.publish(gatewayRegisterTopic.c_str(), s.c_str());
  Serial.printf("[MQTT] Node %d registration forwarded\n", nodeId);
}

// ---------------- LoRa receive handling ----------------
void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  uint8_t pktType = 0;
  LoRa.readBytes(&pktType, 1);

  if (pktType == 0x02) { // RegisterPkt
    RegisterPkt reg;
    reg.pktType = 0x02;
    LoRa.readBytes(((uint8_t*)&reg) + 1, sizeof(reg)-1);
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    Serial.printf("[LoRa] Register from node %d rssi=%d snr=%.1f\n", reg.nodeId, rssi, snr);
    // forward to backend
    if (mqtt.connected()) publishNodeRegister(reg.nodeId, rssi, snr);
    // Optionally send ACK or wait for backend assignment
  } else if (pktType == 0x05) { // STATUS (wrapped PolePacket)
    PolePacket pkt;
    LoRa.readBytes((uint8_t*)&pkt, sizeof(pkt));
    Serial.printf("[LoRa] Status Node %d | Light:%s Fault:%s Time:%02d:%02d RSSI:%d SNR:%d\n",
                  pkt.nodeID, pkt.lightState ? "ON" : "OFF",
                  pkt.fault ? "YES" : "NO",
                  pkt.hour, pkt.minute,
                  pkt.rssi, pkt.snr);
    // forward status to backend (topic: gateway/<id>/node/<nodeId>/status)
    StaticJsonDocument<256> doc;
    doc["type"] = "node_status";
    doc["gatewayId"] = pkt.gatewayID;
    doc["nodeId"] = pkt.nodeID;
    doc["state"] = pkt.lightState ? "ON":"OFF";
    doc["fault"] = pkt.fault;
    doc["time"] = String(pkt.hour) + ":" + String(pkt.minute);
    doc["rssi"] = pkt.rssi;
    doc["snr"] = pkt.snr;
    String s; serializeJson(doc, s);
    String topic = String("gateway/") + String(GATEWAY_ID) + "/node/" + String(pkt.nodeID) + "/status";
    mqtt.publish(topic.c_str(), s.c_str());
    blinkDataLED();
  } else {
    // ignore unknown pktType
    Serial.printf("[LoRa] Unknown pktType %d\n", pktType);
    // flush rest
    while (LoRa.available()) LoRa.read();
  }
}

// ---------------- Beacon broadcast ----------------
void broadcastBeacon() {
  BeaconPkt b;
  b.pktType = 0x01;
  b.gatewayId = GATEWAY_ID;
  b.uptime_s = (uint32_t)(millis() / 1000);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&b, sizeof(b));
  LoRa.endPacket();
  blinkDataLED(10);
}

// ---------------- Main loop ----------------
void loop() {
  // GPRS check
  if (!modem.isGprsConnected()) {
    Serial.println("[SIM900A] GPRS not connected...");
    digitalWrite(LED_CONN, LOW);
    if (gprsFailStart == 0) gprsFailStart = millis();
    if (millis() - gprsFailStart > GPRS_FAIL_TIMEOUT) {
      Serial.println("[SIM900A] GPRS failed timeout. Restarting...");
      ESP.restart();
    }
    if (modem.gprsConnect(APN.c_str(), "", "")) {
      Serial.println("[SIM900A] GPRS Reconnected");
      gprsFailStart = 0;
    } else {
      delay(2000);
      // still attempt LoRa handling even if GPRS down (local)
    }
  } else {
    gprsFailStart = 0;
  }

  // MQTT connection
  if (!mqtt.connected()) {
    if (millis() - lastMqttReconnect > 5000) {
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

  // LoRa receive
  handleLoRaReceive();

  // Beaconing
  if (millis() - lastBeacon >= BEACON_INTERVAL) {
    lastBeacon = millis();
    broadcastBeacon();
  }

  // Telemetry
  if (millis() - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = millis();
    StaticJsonDocument<256> doc;
    doc["type"] = "telemetry";
    doc["gatewayId"] = GATEWAY_ID;
    doc["uptime_s"] = millis() / 1000;
    doc["nodeCount"] = (int)nodeCount;
    String s; serializeJson(doc, s);
    if (mqtt.connected()) mqtt.publish(gatewayStatusTopic.c_str(), s.c_str(), true);
    Serial.println("[MQTT] Telemetry published");
  }

  delay(10); // tiny yield
}

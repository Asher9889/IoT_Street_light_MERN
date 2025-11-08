/*
  Node Firmware (ESP32 + LoRa + RTC + Preferences)
  -----------------------------------------------
  Compatible with the provided Gateway Firmware.
*/

#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>

// -------------------------
// Hardware Pin Config
// -------------------------
#define RELAY_PIN  27
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

// -------------------------
// Globals
// -------------------------
Preferences preferences;
RTC_DS3231 rtc;

String NODE_ID;
String ASSIGNED_GATEWAY;

int lightOnHour = 18, lightOnMin = 0;
int lightOffHour = 6, lightOffMin = 0;

bool lightState = false;
unsigned long lastStatus = 0;
const unsigned long STATUS_INTERVAL = 5 * 60 * 1000UL; // 5 min
unsigned long lastRegister = 0;
const unsigned long REGISTER_INTERVAL = 30 * 1000UL; // 30s

// -------------------------
// Packet Structures (MUST MATCH GATEWAY EXACTLY)
// -------------------------
struct __attribute__((packed)) BeaconPkt {
  uint8_t pktType;
  uint32_t uptime_s;
};

struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType;
  char nodeId[24];
  uint8_t fwVersion;
  uint32_t uptime_s;
};

struct __attribute__((packed)) AssignPkt {
  uint8_t pktType;
  char nodeId[24];
};

struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType;
  char nodeId[24];
  char gatewayId[24];
  uint8_t onHour, onMin;
  uint8_t offHour, offMin;
  uint8_t cfgVer;
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

// -------------------------
// Utility
// -------------------------
String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%012llX", (unsigned long long)chipId);
  return "node" + String(id);
}

// -------------------------
// Preferences
// -------------------------
void loadPreferences() {
  preferences.begin("node-config", true);
  lightOnHour = preferences.getInt("onHour", lightOnHour);
  lightOnMin = preferences.getInt("onMin", lightOnMin);
  lightOffHour = preferences.getInt("offHour", lightOffHour);
  lightOffMin = preferences.getInt("offMin", lightOffMin);
  ASSIGNED_GATEWAY = preferences.getString("gatewayId", "");
  preferences.end();
}

void saveConfig(uint8_t onH, uint8_t onM, uint8_t offH, uint8_t offM, const char* gw) {
  preferences.begin("node-config", false);
  preferences.putInt("onHour", onH);
  preferences.putInt("onMin", onM);
  preferences.putInt("offHour", offH);
  preferences.putInt("offMin", offM);
  preferences.putString("gatewayId", gw);
  preferences.end();
}

// -------------------------
// LoRa Communication
// -------------------------
void sendRegister() {
  RegisterPkt pkt;
  pkt.pktType = 0x02;
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId) - 1);
  pkt.nodeId[sizeof(pkt.nodeId) - 1] = '\0';
  pkt.fwVersion = 1;
  pkt.uptime_s = millis() / 1000;

  LoRa.beginPacket();
  LoRa.write((uint8_t*)&pkt, sizeof(pkt));
  LoRa.endPacket();

  Serial.printf("[NODE] Register sent: %s\n", pkt.nodeId);
}

void sendStatus() {
  DateTime now = rtc.now();
  PolePacket pkt;
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId) - 1);
  pkt.nodeId[sizeof(pkt.nodeId) - 1] = '\0';
  strncpy(pkt.gatewayId, ASSIGNED_GATEWAY.c_str(), sizeof(pkt.gatewayId) - 1);
  pkt.gatewayId[sizeof(pkt.gatewayId) - 1] = '\0';
  pkt.lightState = lightState;
  pkt.fault = false;
  pkt.hour = now.hour();
  pkt.minute = now.minute();
  pkt.rssi = 0;
  pkt.snr = 0;

  LoRa.beginPacket();
  LoRa.write(0x05); // Status type
  LoRa.write((uint8_t*)&pkt, sizeof(pkt));
  LoRa.endPacket();

  Serial.printf("[NODE] Status sent: %s @ %02d:%02d\n", pkt.nodeId, pkt.hour, pkt.minute);
}

void applyConfig(const ConfigPkt& cfg) {
  lightOnHour = cfg.onHour;
  lightOnMin = cfg.onMin;
  lightOffHour = cfg.offHour;
  lightOffMin = cfg.offMin;
  saveConfig(cfg.onHour, cfg.onMin, cfg.offHour, cfg.offMin, cfg.gatewayId);

  Serial.printf("[NODE] Config applied from gateway %s: ON=%02d:%02d OFF=%02d:%02d\n", cfg.gatewayId, cfg.onHour, cfg.onMin, cfg.offHour, cfg.offMin);
}

void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  uint8_t pktType;
  LoRa.readBytes(&pktType, 1);

  if (pktType == 0x03) { // AssignPkt
    AssignPkt ap;
    ap.pktType = pktType;
    LoRa.readBytes(((uint8_t*)&ap) + 1, sizeof(ap) - 1);
    ASSIGNED_GATEWAY = String(ap.nodeId);
    saveConfig(lightOnHour, lightOnMin, lightOffHour, lightOffMin, ASSIGNED_GATEWAY.c_str());
    Serial.printf("[NODE] Assigned to gateway: %s\n", ap.nodeId);
  } 
  else if (pktType == 0x04) { // ConfigPkt
    ConfigPkt cfg;
    cfg.pktType = pktType;
    LoRa.readBytes(((uint8_t*)&cfg) + 1, sizeof(cfg) - 1);
    applyConfig(cfg);
  }
}

// -------------------------
// Light Control
// -------------------------
void updateLightState() {
  DateTime now = rtc.now();
  bool shouldBeOn = false;

  if (lightOnHour > lightOffHour) {
    // crosses midnight
    shouldBeOn = (now.hour() >= lightOnHour || now.hour() < lightOffHour);
  } else {
    shouldBeOn = (now.hour() >= lightOnHour && now.hour() < lightOffHour);
  }

  if (shouldBeOn != lightState) {
    lightState = shouldBeOn;
    digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
    Serial.printf("[NODE] Light turned %s\n", lightState ? "ON" : "OFF");
  }
}

// -------------------------
// Setup & Loop
// -------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Node Boot ===");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  NODE_ID = getDeviceId();
  Serial.printf("[NODE] ID: %s\n", NODE_ID.c_str());

  loadPreferences();

  if (!rtc.begin()) {
    Serial.println("[RTC] Not found");
  }

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433000000)) {
    Serial.println("[LORA] init failed");
    while (1) delay(1000);
  }

  Serial.println("[BOOT] Setup complete");
}

void loop() {
  handleLoRaReceive();
  updateLightState();

  unsigned long now = millis();

  if (now - lastRegister > REGISTER_INTERVAL) {
    lastRegister = now;
    sendRegister();
  }

  if (now - lastStatus > STATUS_INTERVAL) {
    lastStatus = now;
    sendStatus();
  }

  delay(100);
}

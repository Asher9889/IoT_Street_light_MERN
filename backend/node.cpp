/*
  Node Firmware (ESP32 + LoRa + RTC + Preferences)
  -----------------------------------------------
  - Supports dynamic backend configuration
  - Periodic status publishing
  - Handles register, assign, config packets
  - Works with gateway coordination
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

String NODE_ID;  // Unique node identity
String assignedGateway = 0;

// Default Schedule
int lightOnHour = 17, lightOnMin = 58;
int lightOffHour = 6, lightOffMin = 9;

bool prevLightState = false;
bool prevScheduledLightState = false;

unsigned long lastPeriodicPublish = 0;
const unsigned long periodicInterval = 300000UL; // 5 min
unsigned long lastDiscovery = 0;
const unsigned long DISCOVERY_WINDOW = 15000UL;  // 15s discovery

// -------------------------
// Packet Structures (packed)
// -------------------------
struct __attribute__((packed)) BeaconPkt {
  uint8_t pktType;
  uint8_t gatewayId;
  uint32_t uptime_s;
};

struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType;
  char nodeId[13];
  uint8_t fwVersion;
  uint32_t uptime_s;
};

struct __attribute__((packed)) AssignPkt {
  uint8_t pktType;
  char nodeId[13];
  uint8_t gatewayId;
};

struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType;
  char nodeId[13];
  uint8_t gatewayId;
  uint8_t onHour, onMin;
  uint8_t offHour, offMin;
  uint8_t cfgVer;
};

struct __attribute__((packed)) PolePacket {
  char nodeId[13];
  uint8_t gatewayID;
  bool lightState;
  bool fault;
  uint8_t hour;
  uint8_t minute;
  int rssi;
  int snr;
};

// -------------------------
// Utility Functions
// -------------------------

// Generate Unique ID from ESP32 MAC
String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char idStr[13];
  snprintf(idStr, sizeof(idStr), "%012llX", chipId);
  return "node" + String(idStr);
}

// -------------------------
// Preferences
// -------------------------
void loadPreferences() {
  preferences.begin("light-schedule", true);
  lightOnHour  = preferences.getInt("onHour", lightOnHour);
  lightOnMin   = preferences.getInt("onMin", lightOnMin);
  lightOffHour = preferences.getInt("offHour", lightOffHour);
  lightOffMin  = preferences.getInt("offMin", lightOffMin);
  prevLightState = preferences.getBool("lastState", false);
  assignedGateway = preferences.getUChar("assignedGateway", 0);
  preferences.end();
}

void saveAssignedGateway(uint8_t gw) {
  preferences.begin("light-schedule", false);
  preferences.putUChar("assignedGateway", gw);
  preferences.end();
}

// -------------------------
// LoRa Communication
// -------------------------
void sendRegister() {
  RegisterPkt r;
  r.pktType = 0x02;
  strncpy(r.nodeId, NODE_ID.c_str(), sizeof(r.nodeId));
  r.nodeId[sizeof(r.nodeId) - 1] = '\0';
  r.fwVersion = 1;
  r.uptime_s = millis() / 1000;

  LoRa.beginPacket();
  LoRa.write((uint8_t*)&r, sizeof(r));
  LoRa.endPacket();

  Serial.printf("[LoRa] Sent REGISTER node: %s\n", NODE_ID.c_str());
}

void sendStatus(bool actualLightState) {
  uint8_t hour = 0, minute = 0;
  if (rtc.begin()) {
    DateTime now = rtc.now();
    hour = now.hour();
    minute = now.minute();
  }

  PolePacket p;
  strncpy(p.nodeId, NODE_ID.c_str(), sizeof(p.nodeId));
  p.nodeId[sizeof(p.nodeId) - 1] = '\0';
  p.gatewayID = assignedGateway;
  p.lightState = actualLightState;
  p.fault = false;
  p.hour = hour;
  p.minute = minute;
  p.rssi = LoRa.packetRssi();
  p.snr = LoRa.packetSnr();

  LoRa.beginPacket();
  uint8_t t = 0x05;
  LoRa.write(&t, 1);
  LoRa.write((uint8_t*)&p, sizeof(p));
  LoRa.endPacket();

  Serial.printf(">> Node %s sent status\n", NODE_ID.c_str());
}

// -------------------------
// Light Logic
// -------------------------
bool getScheduledLightState() {
  DateTime now;
  if (!rtc.begin()) return false;
  now = rtc.now();

  if (lightOnHour < lightOffHour) {
    return (now.hour() > lightOnHour || (now.hour() == lightOnHour && now.minute() >= lightOnMin)) &&
           (now.hour() < lightOffHour || (now.hour() == lightOffHour && now.minute() < lightOffMin));
  } else {
    return (now.hour() > lightOnHour || (now.hour() == lightOnHour && now.minute() >= lightOnMin)) ||
           (now.hour() < lightOffHour || (now.hour() == lightOffHour && now.minute() < lightOffMin));
  }
}

// -------------------------
// Configuration Apply
// -------------------------
void applyConfigFromPacket(const ConfigPkt &cp) {
  if (strcmp(cp.nodeId, NODE_ID.c_str()) != 0) return;

  lightOnHour = cp.onHour;
  lightOnMin  = cp.onMin;
  lightOffHour = cp.offHour;
  lightOffMin  = cp.offMin;

  preferences.begin("light-schedule", false);
  preferences.putInt("onHour", lightOnHour);
  preferences.putInt("onMin", lightOnMin);
  preferences.putInt("offHour", lightOffHour);
  preferences.putInt("offMin", lightOffMin);
  preferences.end();

  Serial.printf("[Config] Applied schedule %02d:%02d - %02d:%02d\n",
    lightOnHour, lightOnMin, lightOffHour, lightOffMin);
}

// -------------------------
// LoRa Receive Handler
// -------------------------
void handleLoRaPacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  uint8_t pktType = 0;
  LoRa.readBytes(&pktType, 1);

  if (pktType == 0x01) { // Beacon
    BeaconPkt b;
    b.pktType = 0x01;
    LoRa.readBytes(((uint8_t*)&b) + 1, sizeof(b) - 1);
    Serial.printf("[LoRa] Beacon from GW %d\n", b.gatewayId);
    if (assignedGateway == 0) sendRegister();

  } else if (pktType == 0x03) { // Assign
    AssignPkt ap;
    ap.pktType = 0x03;
    LoRa.readBytes(((uint8_t*)&ap) + 1, sizeof(ap) - 1);
    if (strcmp(ap.nodeId, NODE_ID.c_str()) == 0) {
      assignedGateway = ap.gatewayId;
      saveAssignedGateway(assignedGateway);
      Serial.printf("[LoRa] Assigned to gateway %d\n", assignedGateway);
      sendStatus(prevLightState);
    }

  } else if (pktType == 0x04) { // Config
    ConfigPkt cp;
    cp.pktType = 0x04;
    LoRa.readBytes(((uint8_t*)&cp) + 1, sizeof(cp) - 1);
    if (strcmp(cp.nodeId, NODE_ID.c_str()) == 0) {
      applyConfigFromPacket(cp);
      sendStatus(prevLightState);
    }

  } else {
    while (LoRa.available()) LoRa.read(); // flush
  }
}

// -------------------------
// Setup
// -------------------------
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(500);

  NODE_ID = getDeviceId();
  Serial.printf("[Node] Unique ID: %s\n", NODE_ID.c_str());

  Wire.begin(21, 22);
  Wire.setClock(100000);

  if (!rtc.begin()) {
    Serial.println("[RTC] Not found!");
  } else {
    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  loadPreferences();

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(433E6)) {
    Serial.println("[LoRa] Init retry...");
    delay(1000);
  }
  Serial.println("[LoRa] Init OK");

  if (assignedGateway == 0) {
    Serial.println("[DISCOVER] Searching for gateway...");
    unsigned long start = millis();
    while (millis() - start < DISCOVERY_WINDOW) {
      handleLoRaPacket();
      delay(100);
    }
    if (assignedGateway == 0) sendRegister();
  }

  prevScheduledLightState = getScheduledLightState();
  sendStatus(prevLightState);
  lastPeriodicPublish = millis();
}

// -------------------------
// Loop
// -------------------------
void loop() {
  handleLoRaPacket();

  unsigned long now = millis();
  bool scheduled = getScheduledLightState();
  bool rtcChanged = (scheduled != prevScheduledLightState);
  prevScheduledLightState = scheduled;

  bool actualLightState = scheduled;
  digitalWrite(RELAY_PIN, actualLightState ? RELAY_ON : RELAY_OFF);

  if (now - lastPeriodicPublish >= periodicInterval) {
    lastPeriodicPublish = now;
    sendStatus(actualLightState);
  }

  delay(20);
}

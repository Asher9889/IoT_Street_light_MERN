/* ===========================================================
   NODE FIRMWARE (ESP32)
   LoRa-controlled smart streetlight node
   Supports cmdId-based reliable control + ack
   Compatible with new Gateway rewrite (Option A Protocol)
   =========================================================== */

#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>

/* ------------------------ LORA CONFIG ------------------------ */
#define DEFAULT_LORA_FREQ 433000000UL
#define DEFAULT_LORA_SF   7
#define DEFAULT_LORA_BW   125000UL
#define DEFAULT_LORA_CR   5

/* ------------------------ PINS ------------------------ */
#define RELAY_PIN  27
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

/* ------------------------ GLOBALS ------------------------ */
Preferences preferences;
RTC_DS3231 rtc;

String NODE_ID;
String ASSIGNED_GATEWAY;

int lightOnHour = 18, lightOnMin = 0;
int lightOffHour = 6, lightOffMin = 0;
bool lightState = false;

unsigned long lastStatus = 0;
unsigned long lastRegister = 0;
unsigned long REGISTER_INTERVAL = 30000UL;   // 30s
unsigned long STATUS_INTERVAL = 60000UL;     // 60s
bool configured = false;

enum ControlMode {
  AUTO = 0,
  MANUAL_ON = 1,
  MANUAL_OFF = 2
};
ControlMode controlMode = AUTO;

/* ------------------------ PACKETS ------------------------ */
struct __attribute__((packed)) RegisterPkt {
  uint8_t pktType;
  char nodeId[24];
  uint8_t fwVersion;
  uint32_t uptime_s;
};

struct __attribute__((packed)) ConfigPkt {
  uint8_t pktType;
  char nodeId[24];
  char gatewayId[24];
  uint8_t onHour, onMin;
  uint8_t offHour, offMin;
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

/* ---- NEW PROTOCOL MATCHING GATEWAY ---- */
struct __attribute__((packed)) ControlPkt {
  uint8_t pktType; // 0x07
  uint16_t cmdId;
  char nodeId[24];
  bool lightOn;
};

struct __attribute__((packed)) AckPkt {
  uint8_t pktType; // 0x06
  uint16_t cmdId;
  char nodeId[24];
};

/* ------------------------ HELPERS ------------------------ */
volatile bool isLoRaBusy = false;

String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%012llX", (unsigned long long)chipId);
  return "node" + String(id);
}

void sendLoRaPacket(const uint8_t* data, size_t len) {
  if (isLoRaBusy) return;
  isLoRaBusy = true;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.write(data, len);
  LoRa.endPacket(true);
  delay(50);
  LoRa.receive();

  isLoRaBusy = false;
}

/* ------------------------ PERSISTENCE ------------------------ */
void persistModeAndState() {
  preferences.begin("nodecfg", false);
  preferences.putInt("mode", controlMode);
  preferences.putBool("lightState", lightState);
  preferences.end();
}

void loadPreferences() {
  preferences.begin("nodecfg", true);
  ASSIGNED_GATEWAY = preferences.getString("gw", "");
  configured = preferences.getBool("configured", false);
  lightState = preferences.getBool("lightState", false);
  controlMode = (ControlMode)preferences.getInt("mode", AUTO);
  preferences.end();
}

/* ------------------------ CORE ------------------------ */
void applyConfig(const ConfigPkt& cfg) {
  ASSIGNED_GATEWAY = cfg.gatewayId;
  lightOnHour = cfg.onHour;
  lightOnMin  = cfg.onMin;
  lightOffHour = cfg.offHour;
  lightOffMin = cfg.offMin;

  REGISTER_INTERVAL = cfg.regIntervalMs ? cfg.regIntervalMs : REGISTER_INTERVAL;
  STATUS_INTERVAL   = cfg.statusIntervalMs ? cfg.statusIntervalMs : STATUS_INTERVAL;

  preferences.begin("nodecfg", false);
  preferences.putBool("configured", true);
  preferences.putString("gw", ASSIGNED_GATEWAY);
  preferences.end();

  controlMode = AUTO;
  persistModeAndState();

  Serial.printf("[NODE] Config updated (cfgVer=%d)\n", cfg.cfgVer);

  AckPkt ack;
  ack.pktType = 0x06;
  ack.cmdId = cfg.cfgVer; // treating cfgVer same as cmdId for config ack
  strncpy(ack.nodeId, NODE_ID.c_str(), sizeof(ack.nodeId)-1);

  sendLoRaPacket((uint8_t*)&ack, sizeof(ack));
  Serial.println("[NODE] ACK sent for config");
}

void sendRegister() {
  RegisterPkt pkt;
  pkt.pktType = 0x02;
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId)-1);
  pkt.fwVersion = 1;
  pkt.uptime_s = millis() / 1000;
  sendLoRaPacket((uint8_t*)&pkt, sizeof(pkt));
}

void sendStatus() {
  DateTime now = rtc.now();
  PolePacket pkt{};
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId)-1);
  strncpy(pkt.gatewayId, ASSIGNED_GATEWAY.c_str(), sizeof(pkt.gatewayId)-1);
  pkt.lightState = lightState;
  pkt.fault = false;
  pkt.hour = now.hour();
  pkt.minute = now.minute();
  pkt.rssi = 0;
  pkt.snr = 0;

  uint8_t buf[1 + sizeof(PolePacket)] = {0};
  buf[0] = 0x05;
  memcpy(buf + 1, &pkt, sizeof(PolePacket));

  sendLoRaPacket(buf, sizeof(buf));
}

/* ------------------------ CONTROL ------------------------ */
void handleControl(uint8_t header) {
  ControlPkt ctrl;
  ctrl.pktType = header;
  LoRa.readBytes(((uint8_t*)&ctrl) + 1, sizeof(ctrl) - 1);

  ctrl.nodeId[sizeof(ctrl.nodeId)-1] = '\0';
  if (strcmp(ctrl.nodeId, NODE_ID.c_str()) != 0) return;

  lightState = ctrl.lightOn;
  controlMode = ctrl.lightOn ? MANUAL_ON : MANUAL_OFF;
  digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
  persistModeAndState();

  Serial.printf("[NODE] CMD %u → %s\n", ctrl.cmdId, ctrl.lightOn ? "ON" : "OFF");

  delay(100); // required for gateway RX recovery

  AckPkt ack;
  ack.pktType = 0x06;
  ack.cmdId = ctrl.cmdId;
  strncpy(ack.nodeId, NODE_ID.c_str(), sizeof(ack.nodeId)-1);
  sendLoRaPacket((uint8_t*)&ack, sizeof(ack));

  Serial.printf("[NODE] ACK sent for cmdId=%u\n", ctrl.cmdId);
}

/* ------------------------ RADIO ------------------------ */
void handleLoRaReceive() {
  int size = LoRa.parsePacket();
  if (size <= 0) return;

  uint8_t type = LoRa.read();

  if (type == 0x04) {
    ConfigPkt cfg;
    cfg.pktType = type;
    LoRa.readBytes(((uint8_t*)&cfg) + 1, sizeof(cfg) - 1);
    handleControl;
    applyConfig(cfg);
  }

  else if (type == 0x07) {
    handleControl(type);
  }

  else {
    while (LoRa.available()) LoRa.read();
  }
}

/* ------------------------ SCHEDULING ------------------------ */
void updateLightState() {
  if (controlMode != AUTO) return;

  DateTime now = rtc.now();
  bool shouldBeOn;

  if (lightOnHour > lightOffHour)
    shouldBeOn = (now.hour() >= lightOnHour || now.hour() < lightOffHour);
  else
    shouldBeOn = (now.hour() >= lightOnHour && now.hour() < lightOffHour);

  if (shouldBeOn != lightState) {
    lightState = shouldBeOn;
    digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
    persistModeAndState();
  }
}

/* ------------------------ INIT ------------------------ */
void applyLoRaParams() {
  LoRa.end();
  delay(200);
  if (!LoRa.begin(DEFAULT_LORA_FREQ)) {
    Serial.println("[LORA] FAIL");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(DEFAULT_LORA_SF);
  LoRa.setSignalBandwidth(DEFAULT_LORA_BW);
  LoRa.setCodingRate4(DEFAULT_LORA_CR);
  LoRa.enableCrc();
  LoRa.receive();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  NODE_ID = getDeviceId();
  loadPreferences();

  rtc.begin();
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  applyLoRaParams();

  Serial.printf("[NODE] ID=%s\n", NODE_ID.c_str());
}

/* ------------------------ MAIN LOOP ------------------------ */
void loop() {
  handleLoRaReceive();
  updateLightState();

  unsigned long now = millis();

  if (!configured && now - lastRegister > REGISTER_INTERVAL) {
    lastRegister = now;
    sendRegister();
  }

  if (configured && now - lastStatus > STATUS_INTERVAL) {
    lastStatus = now;
    sendStatus();
  }

  delay(10);
}

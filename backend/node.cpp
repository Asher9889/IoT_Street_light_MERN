/* Node Firmware (ESP32 + LoRa + RTC + Preferences)
   Optimized version with persistent config, backend-driven intervals,
   and self-healing registration retry logic.
   Compatible with the provided Gateway Firmware.
*/

#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>

// ---------------- Default LoRa Configuration ----------------
#define DEFAULT_LORA_FREQ 433000000UL
#define DEFAULT_LORA_SF   7
#define DEFAULT_LORA_BW   125000UL
#define DEFAULT_LORA_CR   5

// ---------------- Hardware Pin Config ----------------
#define RELAY_PIN  27
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

// ---------------- Globals ----------------
Preferences preferences;
RTC_DS3231 rtc;

String NODE_ID;
String ASSIGNED_GATEWAY;

int lightOnHour = 18, lightOnMin = 0;
int lightOffHour = 6, lightOffMin = 0;
bool lightState = false;

unsigned long lastStatus = 0;
unsigned long lastRegister = 0;
unsigned long REGISTER_INTERVAL = 30 * 1000UL;     // 30s (initial bootstrap)
unsigned long STATUS_INTERVAL = 1 * 60 * 1000UL;   // default 60s
unsigned long RE_REGISTER_INTERVAL = 10 * 60 * 1000UL; // 4 min (example)

bool configured = false; // persisted state

enum ControlMode {
  AUTO = 0,          // Follows RTC schedule
  MANUAL_ON = 1,     // Forced ON via remote
  MANUAL_OFF = 2     // Forced OFF via remote
};
ControlMode controlMode = AUTO;

// ---------------- LoRa radio parameters ----------------
uint32_t LORA_FREQUENCY = DEFAULT_LORA_FREQ;
uint8_t  LORA_SF = DEFAULT_LORA_SF;
uint32_t LORA_BW = DEFAULT_LORA_BW;
uint8_t  LORA_CR = DEFAULT_LORA_CR;

// ---------------- Packet Structures (MUST MATCH GATEWAY EXACTLY) ----------------
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
  uint32_t regIntervalMs;     // (optional backend-driven)
  uint32_t statusIntervalMs;  // (optional backend-driven)
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
  uint8_t pktType;
  char nodeId[24];
  uint8_t cfgVer;
};

struct __attribute__((packed)) ControlPkt {
  uint8_t pktType; // 0x07
  char nodeId[24];
  char gatewayId[24];
  bool lightOn;
};

// ---------------- Helpers ----------------
String getDeviceId() {
  uint64_t chipId = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%012llX", (unsigned long long)chipId);
  return "node" + String(id);
}

void persistModeAndState() {
  preferences.begin("node-config", false);
  preferences.putInt("mode", (int)controlMode);
  preferences.putBool("lightState", lightState);
  preferences.end();
}

// ---------------- SEND ANY PACKET – ALWAYS RETURN TO RX ----------------
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
  delay(200);          // let TX complete fully

  LoRa.receive();     // re-enable RX mode
  if (!silent) Serial.println("[LORA] Back to RX mode");
  isLoRaBusy = false; // TX complete
}

// ---------------- Persistent Config ----------------
void loadPreferences() {
  preferences.begin("node-config", true); // read-only
  lightOnHour = preferences.getInt("onHour", lightOnHour);
  lightOnMin  = preferences.getInt("onMin", lightOnMin);
  lightOffHour = preferences.getInt("offHour", lightOffHour);
  lightOffMin  = preferences.getInt("offMin", lightOffMin);

  // keep previous API usage of getULong in case you still set those keys elsewhere
  REGISTER_INTERVAL = preferences.getULong("registerInt", REGISTER_INTERVAL);
  STATUS_INTERVAL = preferences.getULong("statusInt", STATUS_INTERVAL);

  ASSIGNED_GATEWAY = preferences.getString("gatewayId", "");
  configured = preferences.getBool("configured", false);

  // load persisted mode & lightState
  int modeInt = preferences.getInt("mode", (int)AUTO);
  controlMode = (ControlMode)modeInt;
  lightState = preferences.getBool("lightState", lightState);

  preferences.end();

  Serial.printf("[CONFIG] Loaded prefs: on=%02d:%02d off=%02d:%02d regInt=%lus statusInt=%lus gateway=%s configured=%d mode=%d lightState=%d\n",
                lightOnHour, lightOnMin, lightOffHour, lightOffMin,
                REGISTER_INTERVAL / 1000, STATUS_INTERVAL / 1000,
                ASSIGNED_GATEWAY.c_str(), configured, (int)controlMode, lightState);
}

void saveConfig(uint8_t onH, uint8_t onM, uint8_t offH, uint8_t offM, const char* gw) {
  preferences.begin("node-config", false);
  preferences.putInt("onHour", onH);
  preferences.putInt("onMin", onM);
  preferences.putInt("offHour", offH);
  preferences.putInt("offMin", offM);
  preferences.putString("gatewayId", gw);
  preferences.putBool("configured", true);
  // don't overwrite mode/lightState here; use persistModeAndState()
  preferences.end();
}

// ---------------- Core Functions ----------------
void applyConfig(const ConfigPkt& cfg) {
  lightOnHour = cfg.onHour;
  lightOnMin  = cfg.onMin;
  lightOffHour = cfg.offHour;
  lightOffMin = cfg.offMin;
  ASSIGNED_GATEWAY = String(cfg.gatewayId);

  // Optional intervals from backend
  if (cfg.regIntervalMs > 0) REGISTER_INTERVAL = cfg.regIntervalMs;
  if (cfg.statusIntervalMs > 0) STATUS_INTERVAL = cfg.statusIntervalMs;

  saveConfig(cfg.onHour, cfg.onMin, cfg.offHour, cfg.offMin, cfg.gatewayId);

  Serial.printf("[NODE] Config applied from GW=%s ON=%02d:%02d OFF=%02d:%02d RegInt=%lus StatusInt=%lus\n",
                cfg.gatewayId, cfg.onHour, cfg.onMin, cfg.offHour, cfg.offMin,
                REGISTER_INTERVAL / 1000, STATUS_INTERVAL / 1000);

  configured = true;

  // When gateway sends schedule/config, return to AUTO mode (explicit, predictable)
  controlMode = AUTO;
  persistModeAndState();

  // Send ACK
  AckPkt ack;
  ack.pktType = 0x06;
  strncpy(ack.nodeId, NODE_ID.c_str(), sizeof(ack.nodeId) - 1);
  ack.nodeId[sizeof(ack.nodeId) - 1] = '\0';
  ack.cfgVer = cfg.cfgVer;

  sendLoRaPacket((uint8_t*)&ack, sizeof(ack));
  Serial.printf("[NODE] Sent ACK for cfgVer=%d\n", ack.cfgVer);
}

void sendRegister() {
  RegisterPkt pkt;
  pkt.pktType = 0x02;
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId) - 1);
  pkt.nodeId[sizeof(pkt.nodeId) - 1] = '\0';
  pkt.fwVersion = 1;
  pkt.uptime_s = millis() / 1000;
  sendLoRaPacket((uint8_t*)&pkt, sizeof(pkt));
  Serial.printf("[NODE] Register sent: %s\n", pkt.nodeId);
}

// void sendStatus() {
//   DateTime now = rtc.now();
//   PolePacket pkt;
//   strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId) - 1);
//   pkt.nodeId[sizeof(pkt.nodeId) - 1] = '\0';
//   strncpy(pkt.gatewayId, ASSIGNED_GATEWAY.c_str(), sizeof(pkt.gatewayId) - 1);
//   pkt.gatewayId[sizeof(pkt.gatewayId) - 1] = '\0';
//   pkt.lightState = lightState;
//   pkt.fault = false;
//   pkt.hour = now.hour();
//   pkt.minute = now.minute();
//   pkt.rssi = 0;
//   pkt.snr = 0;

//   sendLoRaPacket((uint8_t*)&pkt, sizeof(pkt))
//   Serial.printf("[NODE] Status sent: %s @ %02d:%02d (mode=%d)\n", pkt.nodeId, pkt.hour, pkt.minute, (int)controlMode);
// }

void sendStatus() {
  DateTime now = rtc.now();
  PolePacket pkt;
  strncpy(pkt.nodeId, NODE_ID.c_str(), sizeof(pkt.nodeId)-1);
  strncpy(pkt.gatewayId, ASSIGNED_GATEWAY.c_str(), sizeof(pkt.gatewayId)-1);
  pkt.lightState = lightState;
  pkt.fault = false;
  pkt.hour = now.hour();
  pkt.minute = now.minute();
  pkt.rssi = 0;
  pkt.snr = 0;

  // Build full packet: [0x05] + PolePacket
  uint8_t buffer[1 + sizeof(PolePacket)] = {0};
  buffer[0] = 0x05;
  memcpy(buffer + 1, &pkt, sizeof(PolePacket));
  sendLoRaPacket(buffer, sizeof(buffer));
  Serial.printf("[NODE] Status sent: %s @ %02d:%02d (mode=%d)\n",
                pkt.nodeId, pkt.hour, pkt.minute, (int)controlMode);
}
// ---------------- Control packet handling ----------------
// void controlPacket(uint8_t pktType) {
//   ControlPkt ctrl;
//   ctrl.pktType = pktType;

//   // Read remaining bytes from LoRa buffer
//   LoRa.readBytes(((uint8_t*)&ctrl) + 1, sizeof(ctrl) - 1);

//   // Validate target node
//   if (strncmp(ctrl.nodeId, NODE_ID.c_str(), sizeof(ctrl.nodeId)) != 0) {
//     Serial.printf("[NODE] Ignoring control for %s (I am %s)\n", ctrl.nodeId, NODE_ID.c_str());
//     return;
//   }

//   // Apply manual override (persisted). Manual overrides take precedence over schedule.
//   lightState = ctrl.lightOn;
//   controlMode = (ctrl.lightOn ? MANUAL_ON : MANUAL_OFF);

//   digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
//   Serial.printf("[NODE] Light turned %s by gateway command (mode set=%d)\n", lightState ? "ON" : "OFF", (int)controlMode);

//   // Persist the manual override and last state
//   persistModeAndState();

//   // Optional: send immediate status report back to gateway
//   sendStatus();
// }
void controlPacket(uint8_t firstByte) {
  ControlPkt ctrl;
  ctrl.pktType = firstByte;
  LoRa.readBytes(((uint8_t*)&ctrl) + 1, sizeof(ctrl) - 1);

  // Validate target node
  if (strncmp(ctrl.nodeId, NODE_ID.c_str(), sizeof(ctrl.nodeId)) != 0) {
    Serial.printf("[NODE] Ignoring control for %s (I am %s)\n", ctrl.nodeId, NODE_ID.c_str());
    return;
  }

  // Apply manual override (persisted). Manual overrides take precedence over schedule.
  lightState = ctrl.lightOn;
  controlMode = (ctrl.lightOn ? MANUAL_ON : MANUAL_OFF);

  digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
  Serial.printf("[NODE] Light turned %s by gateway command (mode set=%d)\n", lightState ? "ON" : "OFF", (int)controlMode);

  persistModeAndState();

  // SEND ACK (THIS WAS MISSING)
  //delay(350); // so that gateway can return to RX mode
  AckPkt ack;
  ack.pktType = 0x06;
  strncpy(ack.nodeId, NODE_ID.c_str(), sizeof(ack.nodeId)-1);
  ack.nodeId[sizeof(ack.nodeId)-1] = '\0';
  ack.cfgVer = 0;
  sendLoRaPacket((uint8_t*)&ack, sizeof(ack));
  Serial.println("[NODE] ACK sent for CONTROL");

  // // Also send status after ack (optional)
  // delay(30);
  // sendStatus();
}


// ---------------- LoRa Handling ----------------
void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  uint8_t pktType = LoRa.read();

  if (pktType == 0x04) { // Config packet
    ConfigPkt cfg;
    cfg.pktType = pktType;
    LoRa.readBytes(((uint8_t*)&cfg) + 1, sizeof(cfg) - 1);
    if (strncmp(cfg.nodeId, NODE_ID.c_str(), sizeof(cfg.nodeId)) != 0) {
      Serial.printf("[NODE] Ignoring config for %s (I am %s)\n", cfg.nodeId, NODE_ID.c_str());
      return;
    }
    applyConfig(cfg);
  } else if (pktType == 0x07) { // ControlPkt
    controlPacket(pktType);
  } else {
    // other packets: flush
    while (LoRa.available()) LoRa.read();
  }
}

// ---------------- Light Control ----------------
void updateLightState() {
  // Only honor schedule when in AUTO mode
  if (controlMode != AUTO) return;

  DateTime now = rtc.now();
  bool shouldBeOn = false;

  if (lightOnHour > lightOffHour)
    shouldBeOn = (now.hour() >= lightOnHour || now.hour() < lightOffHour);
  else
    shouldBeOn = (now.hour() >= lightOnHour && now.hour() < lightOffHour);

  if (shouldBeOn != lightState) {
    lightState = shouldBeOn;
    digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
    Serial.printf("[NODE] Light turned %s (auto)\n", lightState ? "ON" : "OFF");

    // persist last state (still in AUTO)
    persistModeAndState();
  }
}

// ---------------- LoRa Setup ----------------
void applyLoRaParamsAndStart() {
  LoRa.end();
  delay(200);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[NODE][LORA] init failed");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.enableCrc();
  LoRa.receive();

  Serial.printf("[NODE][LORA] Started (Freq=%lu Hz, SF=%d, BW=%lu Hz, CR=4/%d)\n",
                (unsigned long)LORA_FREQUENCY, LORA_SF, (unsigned long)LORA_BW, LORA_CR);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Node Boot ===");
  Serial.printf("PolePacket size: %d\n", sizeof(PolePacket));
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  NODE_ID = getDeviceId();
  Serial.printf("[NODE] ID: %s\n", NODE_ID.c_str());

  loadPreferences();
  if (!rtc.begin()) Serial.println("[RTC] Not found");

  // Apply persisted physical state immediately (avoid blink)
  digitalWrite(RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
  Serial.printf("[NODE] Restored physical lightState=%d mode=%d\n", lightState, (int)controlMode);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  applyLoRaParamsAndStart();

  if (configured)
    Serial.printf("[NODE] Configured from previous session, assigned to %s\n", ASSIGNED_GATEWAY.c_str());
  else
    Serial.println("[NODE] Unconfigured, will start registration...");

  Serial.println("[BOOT] Setup complete");
}

// ---------------- Loop ----------------
void loop() {
  handleLoRaReceive();
  updateLightState();

  unsigned long now = millis();

  // Registration logic
  if (!configured || ASSIGNED_GATEWAY.isEmpty()) {
    if (now - lastRegister > REGISTER_INTERVAL) {
      lastRegister = now;
      sendRegister();
    }
  } else {
  //   // periodic re-register to maintain link
  //   if (now - lastRegister > RE_REGISTER_INTERVAL) {
  //     lastRegister = now;
  //     sendRegister();
  //     Serial.println("[NODE] Periodic re-registration to maintain link");
  //   }
  // }

  // if (now - lastStatus > STATUS_INTERVAL) {
  //   lastStatus = now;
  //   if (configured && !ASSIGNED_GATEWAY.isEmpty()) {
  //     sendStatus();
  //   } else {
  //     Serial.println("[NODE] Skipping status — not yet configured");
  //   }
  }

  delay(100);
}

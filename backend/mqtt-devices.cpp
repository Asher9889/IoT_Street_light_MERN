#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include <SoftwareSerial.h>
#define RX 16
#define TX 17

SoftwareSerial sim900(RX, TX);
TinyGsm modem(sim900);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char* mqtt_server = "broker.hivemq.com";
const char* register_topic = "iot/devices/register";

String deviceId;
const int RELAY_PINS[10] = {5, 18, 19, 21, 22, 23, 25, 26, 27, 32};

void setup() {
  Serial.begin(115200);
  sim900.begin(9600);

  // Init relays
  for (int i = 0; i < 10; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  deviceId = getDeviceId();
  Serial.println("📟 Device ID: " + deviceId);

  modem.restart();
  modem.gprsConnect("airtelgprs.com", "", "");
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqttCallback);
  connectMQTT();

  // Register device
  sendDeviceRegistration();
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();
}

String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, 13, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  return String("device") + id;
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect(deviceId.c_str())) {
      Serial.println("✅ Connected");
      mqtt.subscribe(("iot/devices/" + deviceId + "/cmd").c_str());
    } else {
      Serial.println("❌ Failed");
      delay(3000);
    }
  }
}

void sendDeviceRegistration() {
  String payload = "{\"deviceId\":\"" + deviceId + "\",\"type\":\"register\",\"relays\":10}";
  mqtt.publish(register_topic, payload.c_str());
  Serial.println("📨 Sent registration: " + payload);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("📥 Received command: " + msg);

  for (int i = 0; i < 10; i++) {
    if (msg[i] == 'O' && msg[i+1] == 'N') digitalWrite(RELAY_PINS[i], HIGH);
    else digitalWrite(RELAY_PINS[i], LOW);
  }
}

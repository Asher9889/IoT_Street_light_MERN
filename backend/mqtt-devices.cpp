#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define MODEM_RX 16
#define MODEM_TX 17

HardwareSerial sim900(1);  // Use UART1 (channel 1)

TinyGsm modem(sim900);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char* mqtt_server = "160.25.62.109";
const int mqtt_port = 1883;
const char* register_topic = "iot/devices/register";

String deviceId;
const int RELAY_PINS[10] = {5, 18, 19, 21, 22, 23, 25, 26, 27, 32};

void setup() {
  Serial.begin(115200);
  delay(1000);

  sim900.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("🔌 Setting up relays...");
  for (int i = 0; i < 10; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  deviceId = getDeviceId();
  Serial.println("📟 Device ID: " + deviceId);

  Serial.println("🔄 Restarting modem...");
  modem.restart();
  delay(2000);

  debugModem();

  Serial.println("📡 Connecting to GPRS...");
  if (!modem.gprsConnect("airtelgprs.com", "", "")) {
    Serial.println("❌ GPRS connection failed. Restarting...");
    while (true); 
  }
  Serial.println("✅ GPRS connected");

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);

  connectMQTT();
  sendDeviceRegistration();
}

void loop() {
  if (!mqtt.connected()) {
    Serial.println("⚠ MQTT disconnected. Reconnecting...");
    connectMQTT();
    sendDeviceRegistration();
  }
  mqtt.loop();
}

// Get unique device ID
String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, 13, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  return String("device") + id;
}

// Debug modem AT info
void debugModem() {
  modem.sendAT("+CSQ");  // Signal quality
  delay(100);
  modem.sendAT("+CCID"); // SIM ID
  delay(100);
  modem.sendAT("+CREG?");// Net reg
  delay(100);
  modem.sendAT("+CGATT?");// GPRS attach
  delay(100);
}

// Connect to MQTT broker
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("🔗 Connecting to MQTT...");
    if (mqtt.connect(deviceId.c_str())) {
      Serial.println("✅ Connected to MQTT broker");
      String subTopic = "iot/devices/" + deviceId + "/cmd";
      mqtt.subscribe(subTopic.c_str());
      Serial.println("📥 Subscribed to: " + subTopic);
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

// Publish device registration info
void sendDeviceRegistration() {
  StaticJsonDocument<200> doc;
  doc["deviceId"] = deviceId;
  doc["type"] = "register";
  doc["relays"] = 10;
  doc["firmware"] = "1.0.0";
  String payload;
  serializeJson(doc, payload);
  mqtt.publish(register_topic, payload.c_str());
  Serial.println("📨 Sent registration: " + payload);
}

// Handle incoming MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📩 Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("⚠ JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey("status") && doc["status"].is<JsonArray>()) {
    JsonArray statusArray = doc["status"].as<JsonArray>();
    for (int i = 0; i < 10 && i < statusArray.size(); i++) {
      String state = statusArray[i];
      digitalWrite(RELAY_PINS[i], state == "ON" ? HIGH : LOW);
      Serial.printf("⚡ Relay %d set to %s\n", i, state.c_str());
    }
  } else if (doc.containsKey("index") && doc.containsKey("status")) {
    int index = doc["index"];
    String state = doc["status"];
    if (index >= 0 && index < 10) {
      digitalWrite(RELAY_PINS[index], state == "ON" ? HIGH : LOW);
      Serial.printf("⚡ Relay %d set to %s\n", index, state.c_str());
    } else {
      Serial.println("⚠ Invalid relay index in command");
    }
  }

  publishStatus();
}

// Publish current status back to broker
void publishStatus() {
  StaticJsonDocument<300> doc;
  doc["deviceId"] = deviceId;
  JsonArray statusArray = doc.createNestedArray("status");
  for (int i = 0; i < 10; i++) {
    statusArray.add(digitalRead(RELAY_PINS[i]) == HIGH ? "ON" : "OFF");
  }
  String topic = "iot/devices/" + deviceId + "/status";
  String payload;
  serializeJson(doc, payload);
  mqtt.publish(topic.c_str(), payload.c_str());
  Serial.println("📤 Status published: " + payload);
}

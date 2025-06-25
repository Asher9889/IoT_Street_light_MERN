#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>  

#define MODEM_RX 16
#define MODEM_TX 17
HardwareSerial sim900(1); // Use UART1

TinyGsm modem(sim900);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

// Replace with your broker and topic
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "home/relays";

const int RELAY_PINS[10] = {5, 18, 19, 21, 22, 23, 25, 26, 27, 32};

void setup() {
  Serial.begin(115200);
  sim900.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);

  for (int i = 0; i < 10; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW); // All OFF initially
  }

  Serial.println("Initializing modem...");
  modem.restart();

  Serial.println("Connecting to network...");
  if (!modem.gprsConnect("airtelgprs.com", "", "")) {
    Serial.println("❌ GPRS connection failed!");
    while (true);
  }

  Serial.println("📶 GPRS connected");

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);

  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();
}

void connectMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!mqtt.connected()) {
    if (mqtt.connect("ESP32_SIM900")) {
      Serial.println("✅ Connected");
      mqtt.subscribe(mqtt_topic);
    } else {
      Serial.print("❌ Failed: ");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📩 MQTT Received: ");
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println(msg);

  StaticJsonDocument<300> doc;
  if (deserializeJson(doc, msg)) {
    Serial.println("⚠️ JSON parse error");
    return;
  }

  for (int i = 0; i < 10; i++) {
    String state = doc[i] | "OFF";
    digitalWrite(RELAY_PINS[i], state == "ON" ? HIGH : LOW);
    Serial.printf("Relay %d: %s\n", i + 1, state.c_str());
  }
}

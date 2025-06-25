#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define SIM900_RX 16
#define SIM900_TX 17
HardwareSerial sim900(1);

const int RELAY_PINS[10] = {5, 18, 19, 21, 22, 23, 25, 26, 27, 32};
String serverURL = "http://a117-103-95-81-184.ngrok-free.app/api/v1/bulb/latest-command";

void setup() {
  Serial.begin(115200);
  sim900.begin(9600, SERIAL_8N1, SIM900_RX, SIM900_TX);

  for (int i = 0; i < 10; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);  // Initially all OFF
  }

  if (checkGSMModule()) {
    Serial.println("✅ GSM Module detected!");
    setupGPRS();
  } else {
    Serial.println("❌ GSM Module not responding.");
    while (true);
  }
}

void loop() {
  String json;
  for (int retry = 0; retry < 3; retry++) {
    json = getCommandFromServer();
    if (json.startsWith("[")) break;
    Serial.println("❌ JSON Not Found in Response!\n🔁 Retrying fetch...");
    delay(3000);
  }

  if (!json.startsWith("[")) {
    Serial.println("❌ Final Attempt Failed. Skipping this cycle.");
    return;
  }

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("⚠️ JSON Parse Error: ");
    Serial.println(error.c_str());
    return;
  }

  for (int i = 0; i < 10; i++) {
    String status = doc[i] | "OFF";
    digitalWrite(RELAY_PINS[i], status == "ON" ? HIGH : LOW);
    Serial.printf("Bulb %d: %s\n", i + 1, status.c_str());
  }

  delay(10000);  // Check every 10 seconds
}

bool checkGSMModule() {
  for (int i = 0; i < 5; i++) {
    sim900.println("AT");
    delay(1000);
    String res = readResponse();
    if (res.indexOf("OK") != -1) return true;
    Serial.println("⏳ Waiting for GSM...");
  }
  return false;
}

void setupGPRS() {
  sendAT("AT+CPIN?");
  sendAT("AT+CREG?");
  sendAT("AT+CGATT=1");
  sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sendAT("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"");
  sendAT("AT+SAPBR=1,1");
  delay(3000);
  sendAT("AT+SAPBR=2,1");
  sendAT("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"");
}

String getCommandFromServer() {
  sendAT("AT+HTTPTERM");
  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"CID\",1");
  sendAT("AT+HTTPPARA=\"URL\",\"" + serverURL + "\"");

  Serial.println("📡 Sending: AT+HTTPACTION=0");
  sim900.println("AT+HTTPACTION=0");
  delay(5000);

  String actionResponse = readResponse();
  if (actionResponse.indexOf("601") != -1 || actionResponse.indexOf("+HTTPACTION:") == -1) {
    Serial.println("❌ HTTPACTION Error (601 or invalid)");
    return "";
  }

  Serial.println("📡 Sending: AT+HTTPREAD");
  sim900.println("AT+HTTPREAD");
  delay(2000);
  String response = readResponse();

  sendAT("AT+HTTPTERM");

  int start = response.indexOf("[");
  int end = response.indexOf("]");

  if (start != -1 && end != -1 && end > start) {
    return response.substring(start, end + 1);
  }
  return "";
}

void sendAT(String cmd) {
  Serial.println("📡 Sending: " + cmd);
  sim900.println(cmd);
  delay(1000);
  String res = readResponse();
  Serial.println(res);
}

String readResponse() {
  String res = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    while (sim900.available()) {
      char c = sim900.read();
      res += c;
    }
  }
  return res;
}

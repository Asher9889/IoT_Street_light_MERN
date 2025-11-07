import mqtt from "mqtt";
import { mqttBrokerUrl } from "../config";

let client: mqtt.MqttClient | null = null;

/**
 * Connects to the MQTT broker and sets up event listeners.
 */
export function initMQTTClient(): mqtt.MqttClient {
  if (client) return client; // prevent double initialization

  console.log("[MQTT] Connecting to broker:", mqttBrokerUrl);

  client = mqtt.connect(mqttBrokerUrl);

  client.on("connect", () => {
    console.log("[MQTT] Connected successfully:", mqttBrokerUrl);
  });

  client.on("reconnect", () => {
    console.log("[MQTT] Reconnecting...");
  });

  client.on("close", () => {
    console.warn("[MQTT] Connection closed.");
  });

  client.on("offline", () => {
    console.warn("[MQTT] Client offline.");
  });

  client.on("error", (err) => {
    console.error("[MQTT] Error:", err.message);
  });

  return client;
}

/**
 * Returns the existing MQTT client instance.
 * Throws if client not initialized.
 */
export function getMQTTClient(): mqtt.MqttClient {
  if (!client) throw new Error("MQTT client not initialized. Call initMQTTClient() first.");
  return client;
}

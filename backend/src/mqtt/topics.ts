import { getMQTTClient } from "./client";
import { handleGatewayRegistration } from "./index";

export function subscribeGatewayTopics() {
  const client = getMQTTClient();

  // Subscribe to gateway registration topic
  const registerTopic = "iot/gateway/+/register";

  client.subscribe(registerTopic, { qos: 1 }, (err) => {
    if (err) console.error("[MQTT] Subscribe error:", err);
    else console.log(`[MQTT] Subscribed to ${registerTopic}`);
  });

  // Handle messages
  client.on("message", (topic, message:Buffer) => {
    if (topic.match(/^iot\/gateway\/.+\/register$/)) {
      try {
        // TODO: find existing gateway in DB & send config
        handleGatewayRegistration(message);

      } catch (e) {
        console.error("[MQTT] Invalid JSON payload:", e);
      }
    }
  });
}

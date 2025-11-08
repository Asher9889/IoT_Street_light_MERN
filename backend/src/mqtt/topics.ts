import { getMQTTClient } from "./client";
import { handleGatewayConfigSet, handleGatewayRegistration } from "./index";

export function subscribeGatewayTopics() {
  const client = getMQTTClient();

  // Subscribe to gateway registration topic
  const registerTopic = "iot/gateway/+/register";

  client.subscribe(registerTopic, { qos: 1 }, (err) => {
    if (err) console.error("[MQTT] Subscribe error:", err);
    else console.log(`[MQTT] Subscribed to ${registerTopic}`);
  });

  // Handle messages
  client.on("message", async (topic, message:Buffer) => {
    if (topic.match(/^iot\/gateway\/.+\/register$/)) {
      try {
        // TODO: find existing gateway in DB & send config
        const gatewayId = await handleGatewayRegistration(message);
        if(!gatewayId) return;
        // Send config to all nodes
        await handleGatewayConfigSet(gatewayId);
      } catch (e) {
        console.error("[MQTT] Invalid JSON payload:", e);
      }
    }
  });
}

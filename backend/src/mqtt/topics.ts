import { logger } from "../logger";
import { getMQTTClient } from "./client";
import { extractDeviceIdFromTopic, handleGatewayBootstrapConfig, handleGatewayConfigSet, handleGatewayRegistration, handleGatewayStatus, handleNodeAck } from "./index";
import { extractGatewayIdFromTopic } from "./utils/extractDeviceIdFromTopic";

export function subscribeGatewayTopics() {
  const client = getMQTTClient();

  // Register
  client.subscribe("iot/gateway/+/register", { qos: 1 });
  // Telemetry / status
  client.subscribe("iot/gateway/+/status", { qos: 1 });
  // for node registration
  client.subscribe("iot/gateway/+/node/+/register", { qos: 1 });
  // for node config ack
  client.subscribe("iot/gateway/+/node/+/config/ack", { qos: 1 });


  client.on("message", async (topic, message: Buffer) => {

    console.log("[MQTT] Received:", topic, message.toString());
    const parts = topic.split("/");

    if (parts.length < 3 || parts[0] !== "iot" || parts[1] !== "gateway") return;

    const gatewayId = parts[2];
    const action = parts[parts.length - 1]; // last segment (e.g., register, status, ack)


    // --- Gateway registration ---
    if (parts.length === 4 && action === "register") {
      const deviceId = extractDeviceIdFromTopic(topic);
      const gatewayId = await handleGatewayRegistration(message);
      if (!gatewayId) return;
      if (!deviceId) return;
      handleGatewayBootstrapConfig(deviceId, gatewayId);
      return;
    }

    // --- Gateway Status/ TELEMETRY ---
    if (parts.length === 4 && action === "status") {
      handleGatewayStatus(topic, message);
      return;
    }

    /**
     * NODE REGISTRATION
     */
    if (parts.includes("node") && action === "register") {
      console.log("[NODE_REGISTER] Node registration topic:", topic);
      await handleGatewayConfigSet(topic, message);
      return;
    }

    // --- Node config ack --- 
    if (parts.includes("node") && action === "ack") {
      await handleNodeAck(topic, message);
      return;
    }

  });
}

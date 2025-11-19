import { logger } from "../logger";
import { getMQTTClient } from "./client";
import { extractDeviceIdFromTopic, handleGatewayBootstrapConfig, handleGatewayConfigSet, handleGatewayRegistration, handleGatewayStatus, handleNodeAck, handleNodeControlAck, identifyTopicType } from "./index";
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

  // for node control ack
  client.subscribe("iot/gateway/+/node/+/control/ack", { qos: 1 });

  //topic = "iot/gateway/" + GATEWAY_ID +
              //"/node/" + String(evt.nodeId) + "/control/ack";


  client.on("message", async (topic, message: Buffer) => {

    const t = identifyTopicType(topic);

    // --- Gateway registration ---
    if (t.isGatewayRegister) {
      const deviceId = extractDeviceIdFromTopic(topic);
      const gatewayId = await handleGatewayRegistration(message);
      if (!gatewayId) return;
      if (!deviceId) return;
      handleGatewayBootstrapConfig(deviceId, gatewayId);
      return;
    }

    // --- Gateway Status/ TELEMETRY ---
    if (t.isGatewayStatus) {
      handleGatewayStatus(topic, message);
      return;
    }

    /**
     * NODE REGISTRATION
     */
    if (t.isNodeRegister) {
      console.log("[NODE_REGISTER] Node registration topic:", topic);
      await handleGatewayConfigSet(topic, message);
      return;
    }

    // --- Node config ack --- 
    if (t.isNodeConfigAck) {
      await handleNodeAck(topic, message);
      return;
    }

    // --- Node control ack ---
    if (t.isNodeControlAck) {
      await handleNodeControlAck(topic, message); // "type":"node_control_ack"
      return;
    }

  });
}

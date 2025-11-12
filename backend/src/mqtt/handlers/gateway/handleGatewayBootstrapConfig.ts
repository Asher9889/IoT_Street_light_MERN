import { getMQTTClient } from "../../client";
import { GatewayMessageType } from "../../interfaces";
import { logger } from "../../../logger";

export default async function handleGatewayBootstrapConfig(deviceId: string, gatewayId: string) {
  const client = getMQTTClient();
  const topic = `iot/gateway/${deviceId}/config/set`;
  const payload = {
    type: GatewayMessageType.DEVICE_CONFIG,
    gatewayId,
    configVersion: 1,
    mqtt: { broker: "103.20.215.109", port: 1883 },
    lora: { frequency: 433000000 },
    apn: "airtelgprs.com",
    nodes: []
  };
  client.publish(topic, JSON.stringify(payload));
  logger.info(`[BOOTSTRAP] Sent config for gateway ${gatewayId} → ${topic} and payload ${JSON.stringify(payload)}`);
}

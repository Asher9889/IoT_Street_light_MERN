import pLimit from "p-limit";
import { Gateway, INode, Node } from "../../../models";
import { getMQTTClient } from "../..";
import { extractGatewayIdFromTopic } from "../../utils/extractDeviceIdFromTopic";
import { extractNodeIdFromTopic, extractDeviceIdFromTopic} from "../../"
import { STATUS } from "../../../constant";
import { GatewayMessageType } from "../../interfaces";
import { logger } from "../../../logger";

export interface INodeRegisterMessage {
  type: GatewayMessageType.NODE_REGISTER;
  deviceId: string;
  gatewayId: string;
  nodeId: string;
  /** Received Signal Strength Indicator from the LoRa packet */
  rssi: number;
  /** Signal-to-Noise Ratio from the LoRa packet */
  snr: number;
  /** Uptime or millisecond timestamp when packet was sent from the gateway */
  timestamp: number;
}


async function publishNodeConfig(node: INode, gatewayId: string) {
  const client = getMQTTClient();
  const payload = {
    type: "node_config",
    nodeId: node.macAddress,
    gatewayId,
    schedule: {
      onHour: node.config.onHour,
      onMin: 0,
      offHour: node.config.offHour,
      offMin: 0,
    },
    configVersion: 1,
  };

  const topic = `iot/gateway/${gatewayId}/node/${node.macAddress}/config/set`;

  return new Promise<void>((resolve, reject) => {
    client.publish(topic, JSON.stringify(payload), (err: any) => {
      if (err) {
        console.error(`[GATEWAY] Failed to send config to node ${node.nodeId}:`, err);
        reject(err);
      } else {
        console.log(JSON.stringify(payload))
        console.log(`[GATEWAY] Config sent to node ${node.nodeId}`);
        logger.info(`[GATEWAY] Config sent to node ${node.nodeId} and payload is ${JSON.stringify(payload)}`);
        resolve();
      }
    });
  });
}

export default async function handleGatewayConfigSet(topic: string, message: Buffer) {
  // const gatewayId = extractGatewayIdFromTopic(topic);
  // const nodeId = extractNodeIdFromTopic(topic);
  try {
  const payload: INodeRegisterMessage = JSON.parse(message.toString());

    if (!payload.gatewayId) return;
    const gateway = await Gateway.findOne({ gatewayId: payload.gatewayId }).lean();
    if (!gateway) {
      console.warn(`[GATEWAY] Config requested for unknown gateway: ${payload.gatewayId}`);
      return;
    }
    let node = await Node.findOneAndUpdate({ macAddress: payload.nodeId }, {
      $set: {
        status: STATUS.ONLINE, 
        lastSeen: new Date(),
        rssi: payload.rssi,
        snr: payload.snr,
      }
    }, {new: true});
    if(!node){
      console.info(`[GATEWAY] No nodes found for gateway ${payload.gatewayId}`);
      return;
    }
    publishNodeConfig(node, payload.gatewayId)

    // const limit = pLimit(5);

    // await Promise.all(nodes.map(node => limit(() => publishNodeConfig(node, gatewayId))));
    // console.log(`[GATEWAY] All configs sent for gateway ${gatewayId}`);
  } catch (err) {
    // console.error(`[GATEWAY] Error handling config set for ${gatewayId}:`, err);
  }
}

/**
 * 
 * 
 * 
 * {"type":"node_register","deviceId":"deviceC0964CBBBC2C","gatewayId":"GW-4","nodeId":"nodeCC29490B65F4","rssi":-50,"snr":9.75,"timestamp":6038125}
 * Nodes to register:  [
  {
    _id: new ObjectId('690f16f56a3cea1f237e9024'),
    nodeId: 'ND-7',
    gatewayId: 'GW-4',
    name: 'Street Light Pole 1',
    macAddress: 'nodeCC29490B65F4',
    status: 'ONLINE',
    lastSeen: 2025-11-08T06:30:00.000Z,
    config: { onHour: 18, offHour: 6, powerLimit: 80 },
    fault: false,
    firmwareVersion: 'v1.2.3',
    createdAt: 2025-11-08T10:09:57.375Z,
    updatedAt: 2025-11-08T10:09:57.375Z,
    __v: 0
  }
]
 */

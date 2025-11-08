import pLimit from "p-limit";
import { Gateway, INode, Node } from "../../../models";
import { getMQTTClient } from "../..";

async function publishNodeConfig(node: INode, gatewayId: string) {
  const client = getMQTTClient();
  const payload = {
    type: "node_config",
    nodeId: node.macAddress,
    gatewayId,
    config: {
      onHour: node.config.onHour,
      onMin: 0,
      offHour: node.config.offHour,
      offMin: 0,
    },
    configVersion: 1,
  };

  const topic = `iot/gateway/${gatewayId}/node/${node.nodeId}/config/set`;

  return new Promise<void>((resolve, reject) => {
    client.publish(topic, JSON.stringify(payload), (err:any) => {
      if (err) {
        console.error(`[GATEWAY] Failed to send config to node ${node.nodeId}:`, err);
        reject(err);
      } else {
        console.log(`[GATEWAY] Config sent to node ${node.nodeId}`);
        resolve();
      }
    });
  });
}

export default async function handleGatewayConfigSet(gatewayId: string) {
    try {
        const gateway = await Gateway.findOne({ macAddress: gatewayId }).lean();
        if (!gateway) {
            console.warn(`[GATEWAY] Config requested for unknown gateway: ${gatewayId}`);
            return;
        }
        const nodes = await Node.find({ gatewayId }).lean();
        if(!nodes.length){
            console.info(`[GATEWAY] No nodes found for gateway ${gatewayId}`);
            return;
        }
        const limit = pLimit(5);

        await Promise.all(nodes.map(node => limit(() => publishNodeConfig(node, gatewayId))));
        console.log(`[GATEWAY] All configs sent for gateway ${gatewayId}`);
    } catch (err) {
        console.error(`[GATEWAY] Error handling config set for ${gatewayId}:`, err);
    }
}

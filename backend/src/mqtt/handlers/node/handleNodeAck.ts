import { logger } from "../../../logger";
import { Node } from "../../../models";

async function handleNodeAck(topic: string, message: Buffer){
    const payload = JSON.parse(message.toString());
    console.log(`[NODE_ACK] Node ${payload.nodeId} acknowledged config v${payload.cfgVer}`);

    await Node.updateOne(
      { macAddress: payload.nodeId },
      {
        $set: {
          lastConfigAck: new Date(),
          configVersion: payload.cfgVer,
        },
      }
    );
    logger.info(`[NODE_ACK] Node ${payload.nodeId} acknowledged. Payload is ${JSON.stringify(payload)}`);
    return;
}

export default handleNodeAck;

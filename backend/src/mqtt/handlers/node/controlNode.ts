import { MODE } from "../../../constant";
import { IControlNode } from "../../../interfaces";
import { logger } from "../../../logger";
import { getMQTTClient } from "../../client";
import { GatewayMessageType } from "../../interfaces";


export function controlNode(nodeData: IControlNode){
    const { gatewayId, nodeId } = nodeData;
    const client = getMQTTClient();
    const topic = `iot/gateway/${gatewayId}/node/${nodeId}/control`;
    const payload = {
        type: nodeData.type, // "node_control",
        cmdId: nodeData.cmdId,  // AUTO, MANUAL        
        gatewayId: nodeData.gatewayId,
        nodeId: nodeData.nodeId,
        action: nodeData.action,
        mode: nodeData.mode
        // for auto
        // "action": "AUTO",
        // "mode": "AUTO" */
    }
    client.publish(topic, JSON.stringify(payload));
    logger.info(`Published node_control command for node ${nodeId} on gateway ${gatewayId} topic: ${topic}`);
}
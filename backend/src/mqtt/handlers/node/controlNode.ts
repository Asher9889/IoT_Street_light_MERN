import { MODE } from "../../../constant";
import { IControlNode } from "../../../interfaces";
import { getMQTTClient } from "../../client";
import { GatewayMessageType } from "../../interfaces";

export function controlNode(nodeData: IControlNode){
    const { gatewayId, nodeId, action } = nodeData;
    const client = getMQTTClient();
    const topic = `iot/gateway/${gatewayId}/node/${nodeId}/control`;
    const payload = {
        type: GatewayMessageType.NODE_CONTROL, // "node_control",
        gatewayId,
        nodeId,
        action,
        mode: MODE.MANUAL // AUTO, MANUAL        
        // for auto
        // "action": "AUTO",
        // "mode": "AUTO" */
    }
    console.log("payload", payload)
    client.publish(topic, JSON.stringify(payload));
}
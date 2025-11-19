import { CommandEntity } from "../../../domain";
import { INodeControlAck } from "../../../interfaces";

const handleNodeControlAck = async (topic: string, message: Buffer) => {
    console.log('ACK Runs===========')
    const payload: INodeControlAck = JSON.parse(message.toString());
    const cmdId = Number(payload.cmdId);

    await CommandEntity.markAck(cmdId);
    // TODO: Use commandLog to update command status in database
    console.log(`[NODE_CONTROL_ACK] Command ${cmdId} acknowledged. Success: ${payload.success}, Timestamp: ${payload.ts}`);
}

export default handleNodeControlAck;

/**
 * Payload is {"type":"node_control_ack","gatewayId":"GW-4","deviceId":"deviceC0964CBBBC2C","nodeId":"nodeCC29490B65F4","cmdId":123,"success":true,"ts":1013391}
 */
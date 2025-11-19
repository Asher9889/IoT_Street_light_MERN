export interface IControlNode {
    type: string;
    cmdId: number;
    gatewayId: string;
    nodeId: string;
    action: string;
    mode: string;
    sentAt: Date;
}

export interface INodeControlAck {
    type: string; // node_control_ack
    nodeId: string;
    gatewayId: string;
    cmdId: string;
    success: boolean;
    ts: number;
}

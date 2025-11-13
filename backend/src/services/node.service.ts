import { StatusCodes } from "http-status-codes";
import { Gateway, Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { mqttService } from "../mqtt";
import { IControlNode } from "../interfaces";


export async function controlNode(nodeData: IControlNode) {
    try {
        const { gatewayId, nodeId } = nodeData;
        const gateway = await Gateway.findOne({ gatewayId }).lean();
        if (!gateway) {
            throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Gateway ${gatewayId} not found`);
        }

        const node = await Node.findOne({ macAddress: nodeId, gatewayId }).lean();
        if (!node) {
            throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Node ${nodeId} not found`);
        }
        mqttService.controlNode(nodeData)
    } catch (error) {
        
    }
}

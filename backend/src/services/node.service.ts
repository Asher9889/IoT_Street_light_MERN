import { StatusCodes } from "http-status-codes";
import { CommandLog, Gateway, Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { mqttService } from "../mqtt";
import { IControlNode } from "../interfaces";
import { CommandEntity } from "../domain";


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
       mqttService.controlNode(nodeData);
       const commandEntity = new CommandEntity(nodeData);
       const createdCommand = await commandEntity.create(commandEntity);
       if(!createdCommand){
        throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, "Failed to create command");
       }
       return createdCommand;
    } catch (error) {
        console.error("Error controlling node:", error);
        throw error;
    }
}

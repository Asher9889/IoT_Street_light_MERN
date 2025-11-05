import { StatusCodes } from "http-status-codes";
import { Gateway, IGateway, INode, Node } from "../models";
import { ApiErrorResponse, getNextGatewaySequence, getNextNodeSequence } from "../utils";
import { deviceApiMessage } from "../utils/api-response";
import { devicesConstant } from "../config";
import validateGatewayData from "../validators/gateway.validator";
import { validateAssignedNodes } from "../validators";

export async function registerGateway(data:IGateway){
    try {
      const isExists = await Gateway.findOne({ $or: [{ gatewayId: data.gatewayId }, { macAddress: data.macAddress }] }).lean();
      if (isExists) {
        throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `${data.gatewayId, data.macAddress} is already registered.`)
      }
      const validNodeIds = await validateAssignedNodes(data.assignedNodes);
      const nextSequence = await getNextGatewaySequence();
      data.gatewayId = nextSequence;
      data.assignedNodes = validNodeIds;
      const result = await new Gateway(data).save();
      if (!result._id) {
        throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, `Failed to save ${data.gatewayId} gateway`)
      }
      return result;
    } catch (error) {
        console.error("Service:registerGateway", error)
        throw error;
    }
}

export async function registerNode(data:INode){
    try {
      const isExists = await Node.findOne({ $or: [{ nodeId: data.nodeId }, { macAddress: data.macAddress }] }).lean();
    if (isExists) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `${data.nodeId, data.macAddress} is already registered.`)
    }
    const gateway = await Gateway.findOne({ gatewayId: data.gatewayId }, { assignedNodes: 1 }).lean();
    if (!gateway) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, deviceApiMessage.gatewayApiMessage.gatewayNotExists(data.gatewayId))
    }
    const nextSequence = await getNextNodeSequence(data.gatewayId);
    data.nodeId = nextSequence;
    if (gateway.assignedNodes.length >= devicesConstant.MAX_NODE_LIMIT) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, deviceApiMessage.nodeApiMessage.limitExceeded(data.gatewayId))
    }  
    const result = await new Node(data).save();
    if (!result._id) {
      throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, `Failed to save ${data.nodeId} node`)
    }
    return result;
    } catch (error) {
      console.error("Service:registerNode", error)
      throw error;
    }
}
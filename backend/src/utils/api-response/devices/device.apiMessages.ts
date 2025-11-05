import { Types } from "mongoose";

export const nodeApiMessage = {
    nodeAlreadyExists: "Node already exists",
    nodeNotExists: (nodeId: number) => `${nodeId} does not exists`,
    nodeSavedSuccessfully: (nodeId: number) => `${nodeId} saved successfully`,
    nodeUpdatedSuccessfully: (nodeId: number) => `${nodeId} updated successfully`,
    nodeDeletedSuccessfully: (nodeId: number) => `${nodeId} deleted successfully`,
    nodeNotDeleted: (nodeId: number) => `${nodeId} not deleted`,
    nodeNotUpdated: (nodeId: number) => `${nodeId} not updated`,
    nodeNotSaved: (nodeId: number) => `${nodeId} not saved`,
    nodeNotFetched: (nodeId: number) => `${nodeId} not fetched`,
    limitExceeded: (gatewayId: number) => `${gatewayId} already has 50 nodes assigned.`,
}

export const gatewayApiMessage = {
    gatewayAlreadyExists:(gatewayId:number) => `Gateway ${gatewayId} already exists`,
    gatewayNotExists: (gatewayId: number) => `Gateway ${gatewayId} does not exists`,
    gatewaySavedSuccessfully: (gatewayId: number) => `Gateway ${gatewayId} saved successfully`,
    gatewayUpdatedSuccessfully: (gatewayId: number) => `Gateway ${gatewayId} updated successfully`,
    gatewayDeletedSuccessfully: (gatewayId: number) => `Gateway ${gatewayId} deleted successfully`,
    gatewayNotDeleted: (gatewayId: number) => `Gateway ${gatewayId} not deleted`,
    gatewayNotUpdated: (gatewayId: number) => `Gateway ${gatewayId} not updated`,
    gatewayNotSaved: (gatewayId: number) => `${gatewayId} not saved`,
    gatewayNotFetched: (gatewayId: number) => `${gatewayId} not fetched`,
}
    
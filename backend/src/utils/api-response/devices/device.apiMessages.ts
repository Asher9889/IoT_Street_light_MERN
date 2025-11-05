import { Types } from "mongoose";

export const nodeApiMessage = {
    nodeAlreadyExists: "Node already exists",
    nodeNotExists: (nodeId: string) => `Node ${nodeId} does not exists`,
    nodeSavedSuccessfully: (nodeId: string) => `Node ${nodeId} saved successfully`,
    nodeUpdatedSuccessfully: (nodeId: string) => `Node ${nodeId} updated successfully`,
    nodeDeletedSuccessfully: (nodeId: string) => `Node ${nodeId} deleted successfully`,
    nodeNotDeleted: (nodeId: string) => `Node ${nodeId} not deleted`,
    nodeNotUpdated: (nodeId: string) => `Node ${nodeId} not updated`,
    nodeNotSaved: (nodeId: string) => `Node ${nodeId} not saved`,
    nodeNotFetched: (nodeId: string) => `Node ${nodeId} not fetched`,
    limitExceeded: (gatewayId: string) => `${gatewayId} already has 50 nodes assigned.`,
}

export const gatewayApiMessage = {
    gatewayAlreadyExists:(gatewayId:string) => `Gateway ${gatewayId} already exists`,
    gatewayNotExists: (gatewayId: string) => `Gateway ${gatewayId} does not exists`,
    gatewaySavedSuccessfully: (gatewayId: string) => `Gateway ${gatewayId} saved successfully`,
    gatewayUpdatedSuccessfully: (gatewayId: string) => `Gateway ${gatewayId} updated successfully`,
    gatewayDeletedSuccessfully: (gatewayId: string) => `Gateway ${gatewayId} deleted successfully`,
    gatewayNotDeleted: (gatewayId: string) => `Gateway ${gatewayId} not deleted`,
    gatewayNotUpdated: (gatewayId: string) => `Gateway ${gatewayId} not updated`,
    gatewayNotSaved: (gatewayId: number) => `${gatewayId} not saved`,
    gatewayNotFetched: (gatewayId: number) => `${gatewayId} not fetched`,
}
    
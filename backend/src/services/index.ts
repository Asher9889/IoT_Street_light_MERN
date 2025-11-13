import { registerGateway, registerNode } from "./device.service";
import { controlNode } from "./node.service";

export const deviceService = {
    registerGateway,
    registerNode,
}


export const nodeService = {
    controlNode
}
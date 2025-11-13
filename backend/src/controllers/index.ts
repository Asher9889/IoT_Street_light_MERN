
import { register, updateAllBulb, updateSingleBulb, registerGateway, registerNode } from "./devices.controller";
import { getLatestCommand, updateBulb } from "./relay.controller";
import { controlNode } from "./node.controller";

const relayController = {
    updateBulb: updateBulb,
    getLatestCommand: getLatestCommand
}

const deviceController = {
    register: register,
    updateAllBulb: updateAllBulb,
    updateSingleBulb: updateSingleBulb,
    registerGateway: registerGateway,
    registerNode: registerNode
}

const nodeController = {
    controlNode
}

export { relayController, deviceController, nodeController }
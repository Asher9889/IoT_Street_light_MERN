
import { register, updateAllBulb, updateSingleBulb, registerGateway, registerNode } from "./devices.controller";
import { getLatestCommand, updateBulb } from "./relay.controller";

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

export { relayController, deviceController }
import { register, updateAllBulb, updateSingleBulb } from "./devices.controller";
import { getLatestCommand, updateBulb } from "./relay.controller";

const relayController = {
    updateBulb: updateBulb,
    getLatestCommand: getLatestCommand
}

const deviceController = {
    register: register,
    updateAllBulb: updateAllBulb,
    updateSingleBulb: updateSingleBulb
}

export { relayController, deviceController }
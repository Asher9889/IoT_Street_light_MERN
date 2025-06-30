import { register } from "./devices.controller";
import { getLatestCommand, updateBulb } from "./relay.controller";

const relayController = {
    updateBulb: updateBulb,
    getLatestCommand: getLatestCommand
}

const deviceController = {
    register: register
}

export { relayController, deviceController }
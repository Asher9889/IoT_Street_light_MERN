import { STATUS } from "../../../constant";
import { Gateway, GatewayLog } from "../../../models";
import { IGatewayRegisterMessage } from "../../../mqtt/interfaces";
import { GatewayMessageType } from "../../interfaces/gateway.interface";

async function handleGatewayRegistration(payload: Buffer) {
    try {
        console.log("payload is:", payload)
        if (!payload) return;
        let data:IGatewayRegisterMessage | GatewayMessageType.DEVICE_OFFLINE;

        if (payload.toString() === GatewayMessageType.DEVICE_OFFLINE) {
            console.info("device is offline")
            return
        }
        data = JSON.parse(payload.toString()) as IGatewayRegisterMessage;
        const gateway = await Gateway.findOne({ macAddress: data.deviceId });
        if(!gateway) return;
        await gateway.updateOne({_id: gateway._id},{
            $set: {
                lastSeen: new Date(),
                firmwareVersion: data.firmwareVersion,
                status: STATUS.ONLINE,
            }
        })
        await GatewayLog.create({
            gatewayId: gateway.gatewayId,
            event: GatewayMessageType.DEVICE_REGISTER,
            message: "Gateway registered successfully",
            timestamp: new Date(),
        })
    } catch (error:any) {
        console.error(error.message);
        throw error;
    }
}

export default handleGatewayRegistration;

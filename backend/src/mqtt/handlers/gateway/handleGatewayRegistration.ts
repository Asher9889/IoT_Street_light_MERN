import { STATUS } from "../../../constant";
import { Gateway, GatewayLog } from "../../../models";
import { IGatewayRegisterMessage } from "../../../mqtt/interfaces";
import { GatewayMessageType } from "../../interfaces/gateway.interface";

async function handleGatewayRegistration(payload: Buffer) {
    try {
        if (!payload) return;
        let data:IGatewayRegisterMessage | GatewayMessageType.DEVICE_OFFLINE;

        if (payload.toString() === GatewayMessageType.DEVICE_OFFLINE) {
            return
        }
        data = JSON.parse(payload.toString()) as IGatewayRegisterMessage;
        const gateway = await Gateway.findOne({ macAddress: data.deviceId }).lean();
        if(!gateway) return;
        await Gateway.updateOne({_id: gateway._id},{
            $set: {
                lastSeen: new Date(),
                firmwareVersion: data.firmwareVersion,
                status: STATUS.ONLINE,
            }
        })
        await GatewayLog.create({
            gatewayId: gateway.gatewayId,
            event: GatewayMessageType.DEVICE_REGISTER,
            message: "Gateway bootstraped successfully",
            timestamp: new Date(),
        })

        return gateway.gatewayId as string;
    } catch (error:any) {
        console.error(error.message);
        throw error;
    }
}

export default handleGatewayRegistration;

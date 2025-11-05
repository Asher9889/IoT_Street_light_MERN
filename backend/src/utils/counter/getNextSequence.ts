import { StatusCodes } from "http-status-codes";
import { devicesConstant } from "../../config";
import { Counter } from "../../models";
import { ApiErrorResponse } from "../api-response";

export async function getNextGatewaySequence(name:string = "gateway"): Promise<number> {
    try {
        const counter = await Counter.findByIdAndUpdate(
            name, // <-- e.g. "gateway"
            { $inc: { seq: 1 } },
            { new: true, upsert: true } // create if missing
        );
        return counter.seq;
    } catch (error: any) {
        throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message);
    }
}

export async function getNextNodeSequence(gatewayId: number): Promise<number> {
    try {
        const key = `node_gateway_${gatewayId}`;
        const counter = await Counter.findByIdAndUpdate(
            key,
            { $inc: { seq: 1 } },
            { new: true, upsert: true }
        );

        if (counter.seq > devicesConstant.MAX_NODE_LIMIT) {
            throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Gateway ${gatewayId} has reached max node limit (${devicesConstant.MAX_NODE_LIMIT}).`);
        }

        return counter.seq;
    } catch (error: any) {
        throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message);
    }
}

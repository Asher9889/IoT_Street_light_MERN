import { NextFunction, Request, Response } from "express";
import { Gateway, Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";
import { nodeService } from "../services";
import { ApiSuccessResponse } from "../utils/api-response";

async function controlNode(req: Request, res: Response, next: NextFunction): Promise<Response | void> {
    try {
        ["type", "gatewayId", "nodeId", "action","mode"].forEach((key) => {
            if (!req.body[key]) {
                throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Please provide valid ${key}`);
            }
        })
        req.body.cmdId = (Date.now() % 65536);
        const payload = await nodeService.controlNode(req.body);
        return res.status(StatusCodes.ACCEPTED).json(new ApiSuccessResponse(StatusCodes.ACCEPTED, "Command queued. Awaiting device ACK", payload));
    } catch (error: any) {
        console.error("Error in controlNode:", error);
        if (error instanceof ApiErrorResponse) {
            return next(error)
        }
        return next(new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message));
    }
}

export { controlNode }

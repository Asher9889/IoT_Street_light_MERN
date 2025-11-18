import { NextFunction, Request, Response } from "express";
import { Gateway, Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";
import { nodeService } from "../services";
import { ApiSuccessResponse } from "../utils/api-response";

async function controlNode(req: Request, res: Response, next: NextFunction): Promise<Response | void>{
    try {
        const { gatewayId, nodeId, action, cmdId, mode } = req.body;

        ["gatewayId", "nodeId", "action", "cmdId", "mode"].forEach((key) => {
            if(!req.body[key]){
                throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Please provide valid ${key}`);
            }
        })

        await nodeService.controlNode({ gatewayId, nodeId, action, cmdId, mode });

        return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, "Command send to node successfully"));
        
        
    } catch (error: any) {
        if (error instanceof ApiErrorResponse) {
            return next(error)
        }
        return next(new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message));
    }
}

export { controlNode }

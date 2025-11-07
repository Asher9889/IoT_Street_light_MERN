import { NextFunction, Request, Response } from "express";
import { Device } from "../models";
import { ApiErrorResponse,  registerDeviceSchema, deviceApiMessage } from "../utils";
import { StatusCodes } from 'http-status-codes';
import { ApiSuccessResponse } from "../utils/api-response";
// import { publishSingleBulb } from "../utils/mqtt/mqttClient";
import { registerGatewaySchema, validateNodeData } from "../validators";
import validateGatewayData from "../validators/gateway.validator";
import { deviceService } from "../services";


//---------- Register One Pair of Device----->
async function register(req: Request, res: Response, next: NextFunction): Promise<Response | void> {
  try {
    const { error, value } = registerDeviceSchema.validate(req.body)

    if (error) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, error.message)
    }
    req.body = value;

    const isExists = await Device.findOne({ deviceId: req.body.deviceId });
    console.log("isExists: ", isExists)

    if (isExists) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `${req.body.deviceId} is already registered.`)
    }

    const data = new Device(req.body);

    const result = await data.save();
    if (!result._id) {
      throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, `Failed to save ${req.body.deviceId} device`)
    }

    return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, `${req.body.deviceId} saved successfully`, result))

  } catch (error: any) {
    if (error instanceof ApiErrorResponse) {
      next(error)
    } else {
      const statusCode = StatusCodes.INTERNAL_SERVER_ERROR;
      const msg = error.message || "INTERNAL SERVER ERROR";
      next(new ApiErrorResponse(statusCode, msg));
    }
  }


}

//------- update Bulb (Command)------->
async function updateAllBulb(req: Request, res: Response, next: NextFunction): Promise<Response | undefined> {
  try {
    const { deviceId } = req.params;
    const { status } = req.body;

    if (!Array.isArray(status)) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Please provide valid status`)

    }
    if (!deviceId) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Please provide valid deviceId`)
    }

    const device = await Device.findOne({ deviceId: deviceId });
    if (!device) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `${deviceId} does not exists.`)
    }
    device.status = status;
    device.lastSeen = new Date();

    await device.save();

    // publishBulbStates(deviceId, status);
    return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, "Successfully updated the bulb", status));

  } catch (error: any) {
    if (error instanceof ApiErrorResponse) {
      next(error)
    } else {
      const statusCode = StatusCodes.BAD_GATEWAY;
      const msg = error.message;
      next(new ApiErrorResponse(statusCode, msg));
    }
  }
}

//-------- update Single Bulb------->
async function updateSingleBulb(req: Request, res: Response, next: NextFunction): Promise<Response | undefined> {
  try {
    const { deviceId } = req.params;
    const { index, status } = req.body;

    if (!deviceId || typeof index !== 'number' || !['ON', 'OFF'].includes(status)) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Invalid input data`);
    }

    const device = await Device.findOne({ deviceId: deviceId });
    if (!device) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `${deviceId} does not exist.`);
    }

    if (index < 0 || index >= device.relayCount) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Invalid relay index`);
    }

    device.status[index] = status;
    device.lastSeen = new Date();
    await device.save();

    // publishSingleBulb(deviceId, index, status);

    return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, "Bulb updated", { index, status }));
  } catch (error: any) {
    if (error instanceof ApiErrorResponse) {
      next(error)
    } else {
      next(new ApiErrorResponse(StatusCodes.BAD_GATEWAY, error.message));
    }
  }
}

async function registerGateway(req: Request, res: Response, next: NextFunction): Promise<Response | void> {
  try {
    const validatedData = validateGatewayData(req.body);
    const result = await deviceService.registerGateway(validatedData);
    return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, `${validatedData.gatewayId} saved successfully`, result))
  
  } catch (error: any) {
    if (error instanceof ApiErrorResponse) {
      return next(error)
    }
    return next(new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message));
  }
}

async function registerNode(req: Request, res: Response, next: NextFunction): Promise<Response | void> {
  try {
    const validatedData = validateNodeData(req.body);
    validatedData.nodeId = `ND-${validatedData.nodeId}`;
    const result = await deviceService.registerNode(validatedData);
    return res.status(StatusCodes.OK).json(new ApiSuccessResponse(StatusCodes.OK, deviceApiMessage.nodeApiMessage.nodeSavedSuccessfully(result.nodeId), result))
  } catch (error: any) {
    if (error instanceof ApiErrorResponse) {
      return next(error)
    }
    return next(new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message));
  }
}



export { register, updateAllBulb, updateSingleBulb, registerGateway, registerNode }
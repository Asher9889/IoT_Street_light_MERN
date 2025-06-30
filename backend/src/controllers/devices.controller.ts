import { NextFunction, Request, Response } from "express";
import { Device, IDeviceSchema } from "../models";
import { ApiErrorResponse, registerDeviceSchema } from "../utils";
import { StatusCodes } from 'http-status-codes';

//---------- Register One Pair of Device----->
async function register(req: Request, res: Response, next: NextFunction) {
  try {
    const { error, value } = registerDeviceSchema.validate(req.body)
    
    console.log(error, value)
    
    if (error) {
      console.log("error.message: ", error.message)
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, error.message)
    }
    req.body = value;

    const isExists = await Device.find({ deviceId: req.body.deviceId });
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

export { register }
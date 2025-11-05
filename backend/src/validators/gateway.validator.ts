import { StatusCodes } from "http-status-codes";
import Joi from "joi";
import { ApiErrorResponse } from "../utils";
import { IGateway } from "../models";
import { devicesConstant } from "../config";

const registerGatewaySchema = Joi.object({
  gatewayId: Joi.number()
    .integer()
    .positive()
    .optional()
    .messages({
      "number.base": "gatewayId must be a number",
      "number.positive": "gatewayId must be positive",
      "any.required": "gatewayId is required",
    }),

  macAddress: Joi.string()
    .trim()
    .min(8)
    .required()
    .messages({
      "string.base": "macAddress must be a string",
      "string.min": "Invalid MAC address",
      "any.required": "macAddress is required",
    }),

  name: Joi.string().trim().min(2).max(50).required().messages({
    "string.min": "Name must be at least 2 characters",
    "any.required": "name is required",
  }),

  mqttTopic: Joi.string().trim().min(5).max(50).required().messages({
    "string.min": "mqttTopic must be at least 5 characters long",
    "any.required": "mqttTopic is required",
  }),

  location: Joi.object({
    lat: Joi.number().required().messages({
      "number.base": "lat must be a number",
      "any.required": "lat is required",
    }),
    lng: Joi.number().required().messages({
      "number.base": "lng must be a number",
      "any.required": "lng is required",
    }),
    address: Joi.string().trim().required().messages({
      "any.required": "address is required",
    }),
  }).required(),

  lora: Joi.object({
    frequency: Joi.number().required(),
    bandwidth: Joi.number().required(),
    spreadingFactor: Joi.number().required(),
  }).required(),
  assignedNodes: Joi.array().items(Joi.string()).max(devicesConstant.MAX_NODE_LIMIT),

  network: Joi.object({
    simIccid: Joi.string().required(),
    apn: Joi.string().optional(),
    ipAddress: Joi.string().optional(),
  }).required(),
  configVersion: Joi.string().required(),
});

export default function validateGatewayData(gatewayData: IGateway) {
   const {error, value} =  registerGatewaySchema.validate(gatewayData)
   if(error){
    throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, error.message)
   }
   return value;
}

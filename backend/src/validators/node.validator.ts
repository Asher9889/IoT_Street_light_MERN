import Joi from "joi";
import { STATUS } from "../constant";
import { INode } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";


const nodeSchema = Joi.object({
  nodeId: Joi.string().optional()
    .messages({
      "any.required": "nodeId is required",
    }),

  gatewayId: Joi.string().required()
    .required()
    .messages({
      "any.required": "gatewayId is required",
    }),

  name: Joi.string()
    .trim()
    .min(2)
    .max(50)
    .required()
    .messages({
      "string.base": "name must be a string",
      "string.empty": "name cannot be empty",
    }),

  macAddress: Joi.string()
    .trim()
    // .pattern(/^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$/)
    .required()
    .messages({
      // "string.pattern.base": "macAddress must be a valid MAC (e.g. 24:6F:28:9A:4B:CD)",
      "any.required": "macAddress is required",
    }),

  status: Joi.string()
    .valid(...Object.values(STATUS))
    .default(STATUS.OFFLINE),

  lastSeen: Joi.date().optional().allow(null),

  config: Joi.object({
    onHour: Joi.number()
      .min(0)
      .max(23)
      .required()
      .messages({
        "number.base": "onHour must be a number (0–23)",
      }),
    offHour: Joi.number()
      .min(0)
      .max(23)
      .required()
      .messages({
        "number.base": "offHour must be a number (0–23)",
      }),
    powerLimit: Joi.number()
      .positive()
      .max(10000)
      .required()
      .messages({
        "number.base": "powerLimit must be a number",
      }),
  })
    .required()
    .messages({
      "any.required": "config is required",
    }),

  fault: Joi.boolean().default(false),

  firmwareVersion: Joi.string()
    .trim()
    .pattern(/^v?\d+\.\d+(\.\d+)?$/)
    .required()
    .messages({
      "string.pattern.base": "firmwareVersion must follow semantic versioning (e.g. 1.0.0)",
    }),
});

export default function validateNodeData(data: INode) {
  const { error, value } = nodeSchema.validate(data);
  if(error){
    throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, error.message)
   }
   return value;
}
  
import Joi from "joi";
import { LIGHT_STATE, MODE, STATUS } from "../constant";
import { INode } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";


const nodeSchema = Joi.object({
  nodeId: Joi.string().optional(),

  gatewayId: Joi.string().required()
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

  lightState:{
    status: Joi.string()
    .valid(...Object.values(LIGHT_STATE))
    .default(LIGHT_STATE.OFF),
    updatedAt: Joi.date().default(null),
  },

  lastSeen: Joi.date().optional().allow(null),

  config: Joi.object({
    onHour: Joi.number()
      .min(0)
      .max(23)
      .default(16)
      .messages({
        "number.base": "onHour must be a number (0–23)",
      }),
    offHour: Joi.number()
      .min(0)
      .max(23)
      .default(6)
      .messages({
        "number.base": "offHour must be a number (0–23)",
      }),
    powerLimit: Joi.number()
      .positive()
      .max(10000)
      .default(80)
      .messages({
        "number.base": "powerLimit must be a number",
      }),
  }).default()
    .messages({
      "any.required": "config is required",
    }), 

  intervals: Joi.object({
    register: Joi.number()
      .positive()
      .default(600000), // seconds 10 mins
    status: Joi.number()
      .positive()
      .default(60000), // seconds 1 min
  }).default()
    .messages({
      "any.required": "intervals is required",
    }),

  mode: Joi.string()
    .valid(...Object.values(MODE))
    .default(MODE.AUTO),
  rssi: Joi.number().allow(null).default(null),
  snr: Joi.number().allow(null).default(null),
  lastConfigAck: Joi.date().allow(null).default(null),
  configVersion: Joi.number().allow(null).default(null),

  fault: Joi.boolean().allow(null).default(false),

  firmwareVersion: Joi.string()
    .trim()
    .pattern(/^v?\d+\.\d+(\.\d+)?$/)
    .default("1.0.0")
    .messages({
      "string.pattern.base": "firmwareVersion must follow semantic versioning (e.g. 1.0.0)",
    }),
});

export default function validateNodeData(data: INode) {
  const { error, value } = nodeSchema.validate(data);
  if (error) {
    throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, error.message)
  }
  return value;
}

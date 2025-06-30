import Joi from "joi";

const registerDeviceSchema = Joi.object({
    deviceId: Joi.string().required(),
    name: Joi.string().optional(),
    simNumber: Joi.string().optional(),
    relayCount: Joi.number().integer().min(1).max(20).default(10),
    status: Joi.array().items(Joi.string().valid("ON", "OFF")).default(() => Array(10).fill("OFF")),
    registeredAt: Joi.date().required(),
    lastSeen: Joi.date().required(),
    location: Joi.object({
        lat: Joi.number().required(),
        long: Joi.number().required(),
    }).optional(),
    isActive: Joi.boolean().default(false),
});

export default registerDeviceSchema;
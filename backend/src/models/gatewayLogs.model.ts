import mongoose, { Schema, Document } from "mongoose";
import { GatewayMessageType } from "../mqtt/interfaces";

export const EventLogsLevelEnum = ["debug", "info", "warn", "error"];
export interface IGatewayLog extends Document {
  gatewayId: string;
  event: GatewayMessageType;
  level: (typeof EventLogsLevelEnum)[number];
  message: string;
  payload?: Record<string, unknown>;
  meta?: Record<string, unknown>;
  timestamp: Date;
}

const GatewayLogSchema = new Schema<IGatewayLog>(
  {
    gatewayId: { type: String, ref: "Gateway", required: true },

    event: { type: String, required: true, enum: GatewayMessageType },

    level: {
      type: String,
      enum: EventLogsLevelEnum,
      default: "info",
    },

    message: { type: String, required: true },

    // Optional structured data
    payload: {
      type: Schema.Types.Mixed,
      default: {},
    },

    // Extra arbitrary metadata
    meta: {
      type: Schema.Types.Mixed,
      default: {},
    },

    timestamp: {
      type: Date,
      default: Date.now,
      index: true,
    },
  },
  { timestamps: false }
);

// OPTIONAL: TTL index - delete logs older than 30 days
// GatewayLogSchema.index({ timestamp: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 30 });

const GatewayLog = mongoose.model<IGatewayLog>("GatewayLog", GatewayLogSchema);
export default GatewayLog;

import { Schema, model, Document } from "mongoose";
import { COMMAND_STATUS, LIGHT_STATE, TLightState, TCommandStatus } from "../constant";

export interface ICommandLog extends Document {
  type: string;
  cmdId: number;               // 0–65535 from LoRa
  nodeId: string;
  gatewayId: string;
  action: TLightState;         // "ON" | "OFF" | ...
  status: TCommandStatus;      // "PENDING" | "ACKED" | "FAILED" | "EXPIRED"
  success: boolean | null;     // null = not resolved yet
  ackAt?: Date | null;
  sentAt: Date;
  rawPayload?: Record<string, any>;
}

const CommandLogSchema = new Schema<ICommandLog>(
  {
    type: { type: String, enum: ["node_control"], required: true },
    cmdId: { type: Number, required: true, min: 0, max: 65535, index: true },
    nodeId: { type: String, required: true, index: true },
    gatewayId: { type: String, required: true, index: true },
    action: { type: String, enum: Object.values(LIGHT_STATE), required: true },
    status: { type: String, enum: Object.values(COMMAND_STATUS), default: COMMAND_STATUS.PENDING, index: true },
    success: { type: Boolean, default: null },
    ackAt: { type: Date, default: null },
    sentAt: { type: Date, required: true },
    rawPayload: { type: Schema.Types.Mixed, default: {} },
  }, { timestamps: true, versionKey: false }
);

CommandLogSchema.index({ createdAt: 1 }, { expireAfterSeconds: 60 * 60 * 24 * 30 }); //30 days

const CommandLog = model("CommandLog", CommandLogSchema);

export default CommandLog;
 
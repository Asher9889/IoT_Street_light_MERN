// models/GatewayLog.ts
import mongoose, { Schema, Document, Types } from "mongoose";
import { GatewayMessageType } from "../mqtt/interfaces";

export interface IGatewayLog extends Document {
  gatewayId: string;
  event: GatewayMessageType;
  message: string;
  timestamp: Date;
}
 
const GatewayLogSchema = new Schema<IGatewayLog>({
  gatewayId: { type: String, ref: "Gateway", required: true },
  event: { type: String, required: true },
  message: { type: String, required: true },
  timestamp: { type: Date, default: Date.now },
});

const GatewayLog = mongoose.model<IGatewayLog>("GatewayLog", GatewayLogSchema);

export default GatewayLog;

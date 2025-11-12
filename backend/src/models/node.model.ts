import mongoose, { Schema, Document, Model, Types } from "mongoose";
import { STATUS, Status } from "../constant";

export interface INode extends Document {
  _id: Types.ObjectId;
  nodeId: string;
  gatewayId: string; // ref to Gateway
  name: string;
  macAddress: string;
  status: Status;
  lastSeen: Date;
  config: {
    onHour: number;
    offHour: number;
    powerLimit: number;
  };
  rssi?: number;
  snr?: number;
  lastConfigAck?: Date;
  configVersion?: number;
  fault: boolean;
  firmwareVersion: string;
  createdAt: Date;
  updatedAt: Date;
}

const NodeSchema = new Schema<INode>(
  {
    nodeId: { type: String, required: true, unique: true},
    gatewayId: { type: Schema.Types.String, ref: "Gateway", required: true },
    name: { type: String, required: true },
    macAddress: { type: String, required: true, unique: true },
    status: {
      type: String,
      enum: Object.values(STATUS), // ["ONLINE", "OFFLINE"]
      default: STATUS.OFFLINE,
    },
    lastSeen: { type: Date, default: null },
    config: {
      onHour: { type: Number, required: true, default: 18 },
      offHour: { type: Number, required: true, default: 6 },
      powerLimit: { type: Number, required: true, default: 80 }
    },
    rssi: { type: Number, default: null },
    snr: { type: Number, default: null },
    lastConfigAck: { type: Date, default: null },
    configVersion: { type: Number, default: null },
    fault: { type: Boolean, default: false },
    firmwareVersion: { type: String, required: true },
  },
  {
    timestamps: true, // adds createdAt & updatedAt automatically
  }
);

// ✅ Indexes for faster lookup
NodeSchema.index({ nodeId: 1 });
NodeSchema.index({ macAddress: 1 });
NodeSchema.index({ gatewayId: 1 });

const Node= mongoose.model<INode>("Node", NodeSchema);
export default Node;

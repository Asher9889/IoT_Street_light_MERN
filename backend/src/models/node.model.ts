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
  fault: boolean;
  firmwareVersion: string;
  createdAt: Date;
  updatedAt: Date;
}

const NodeSchema = new Schema<INode>(
  {
    nodeId: { type: String, required: true},
    gatewayId: { type: Schema.Types.String, ref: "Gateway", required: true },
    name: { type: String, required: true },
    macAddress: { type: String, required: true },
    status: {
      type: String,
      enum: Object.values(STATUS), // ["ONLINE", "OFFLINE"]
      default: STATUS.OFFLINE,
    },
    lastSeen: { type: Date, default: null },
    config: {
      onHour: { type: Number, required: true },
      offHour: { type: Number, required: true },
      powerLimit: { type: Number, required: true },
    },
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

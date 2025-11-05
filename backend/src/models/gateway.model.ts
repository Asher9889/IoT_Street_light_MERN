import mongoose, { Document, Schema, Types, Model } from "mongoose";
import { Status, STATUS } from "../constant";

export interface IGateway extends Document {
  gatewayId: number;
  macAddress: string;
  name: string;
  status: Status;
  lastSeen: Date;
  mqttTopic: string; // e.g. "backend/config/<mac>"
  location: {
    lat: number;
    lng: number;
    address: string;
  };
  lora: {
    frequency: number;
    bandwidth: number;
    spreadingFactor: number;
  };
  network: {
    simIccid: string;
    apn: string;
    ipAddress: string;
  };
  assignedNodes: Types.ObjectId[];
  configVersion: string;
}

const GatewaySchema = new Schema<IGateway>(
  {
    gatewayId: { type: Number, required: true },
    macAddress: { type: String, required: true },
    name: { type: String, required: true },
    status: { type: String, enum: [STATUS.OFFLINE, STATUS.ONLINE], default: STATUS.OFFLINE },
    lastSeen: { type: Date, default: null },
    mqttTopic: { type: String, required: true }, // "backend/config/<mac>"

    location: {
      lat: { type: Number, required: true },
      lng: { type: Number, required: true },
      address: { type: String, required: true },
    },

    lora: {
      frequency: { type: Number, required: true },
      bandwidth: { type: Number, required: true },
      spreadingFactor: { type: Number, required: true },
    },

    network: {
      simIccid: { type: String },
      apn: { type: String },
      ipAddress: { type: String },
    },

    assignedNodes: {
      type: [{ type: Schema.Types.ObjectId, ref: "Node" }],
      validate: {
        validator: function (val: Types.ObjectId[]) {
          return !val || val.length <= 50;
        },
        message: "A gateway cannot have more than 50 assigned nodes.",
      },
    },


    configVersion: { type: String, default: "v1" },
  },
  {
    timestamps: true, // automatically manages createdAt and updatedAt
  }
);
 
// Recommended indexes
GatewaySchema.index({ macAddress: 1 });
GatewaySchema.index({ gatewayId: 1 });
GatewaySchema.index({ "location.lat": 1, "location.lng": 1 });

const Gateway: Model<IGateway> = mongoose.model<IGateway>("Gateway", GatewaySchema);

export default Gateway;

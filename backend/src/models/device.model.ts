import mongoose, { Document } from "mongoose";

export interface IDeviceSchema extends Document {
    deviceId: string;
    name?: string;
    simNumber?: string;
    relayCount: number;
    status: string[];
    registeredAt: Date;
    lastSeen: Date;
    location?: {
        lat: number;
        lng: number;
    };
    isActive: boolean;
}

const deviceSchema = new mongoose.Schema<IDeviceSchema>({
    deviceId: {
        type: String,
        required: true,
        unique: true,
    },
    name: {
        type: String,
    },
    simNumber: {
        type: String,
    },
    relayCount: {
        type: Number,
        default: 10,
    },
    status: {
        type: [String],
        default: Array(10).fill("OFF"),
    },
    registeredAt: {
        type: Date,
        default: Date.now,
    },
    lastSeen: {
        type: Date,
        default: Date.now,
    },
    location: {
        lat: Number,
        long: Number,
    },
    isActive: {
        type: Boolean,
        default: true,
    },
});

const Device = mongoose.model<IDeviceSchema>("Device", deviceSchema);

export default Device;

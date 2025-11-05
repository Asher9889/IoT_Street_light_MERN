import mongoose, { Schema, Document } from "mongoose";

export interface ICounter extends Document {
  _id: string; // e.g., "gateway" or "node_gateway_1"
  seq: number; // last used ID number
}

const CounterSchema = new Schema<ICounter>({
  _id: { type: String, required: true },
  seq: { type: Number, default: 0 },
});

const Counter = mongoose.model<ICounter>("Counter", CounterSchema);

export default Counter;

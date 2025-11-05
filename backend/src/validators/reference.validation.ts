import mongoose, { Types } from "mongoose";
import { Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";

export async function validateAssignedNodes(nodeIds: Types.ObjectId[]) {
  try {
    if (!Array.isArray(nodeIds) || nodeIds.length === 0) return [];
  
    // Step 1: Filter invalid ObjectIds
    const invalidIds = nodeIds.filter((id) => !mongoose.Types.ObjectId.isValid(id));
    if (invalidIds.length) {
      throw new ApiErrorResponse(400, `Invalid ObjectId(s): ${invalidIds.join(", ")}`);
    }
  
    // Step 2: Check existence
    const existing = await Node.find({ _id: { $in: nodeIds } }, {_id: 1}).lean();
    const existingIds = existing.map((n) => n._id);
  
    // Step 3: Collect missing ones
    const missingIds = nodeIds.filter((id) => !existingIds.includes(id));
    if (missingIds.length) {
      throw new ApiErrorResponse(400, `Nodes not found: ${missingIds.join(", ")}`);
    }
  
    return existingIds; // normalized valid list
  } catch (error:any) {
    if(error instanceof ApiErrorResponse){
      throw error;
    }
    throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message);
  }
}

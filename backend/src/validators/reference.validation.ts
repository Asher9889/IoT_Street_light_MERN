import mongoose, { Types } from "mongoose";
import { Node } from "../models";
import { ApiErrorResponse } from "../utils";
import { StatusCodes } from "http-status-codes";

export async function validateAssignedNodes(nodeIds: string[]) {
  try {
    if (!Array.isArray(nodeIds) || nodeIds.length === 0) return [];
  
    // Step 1: Filter invalid ObjectIds
    const invalidIds = nodeIds.filter((nodeId:string) => !nodeId.startsWith("ND-"));
    if (invalidIds.length) {
      throw new ApiErrorResponse(400, `Invalid ObjectId(s): ${invalidIds.join(", ")}`);
    }
  
    // Step 2: Check existence
    const existing = await Node.find({ nodeId: { $in: nodeIds } }, { nodeId: 1 }).lean();
    const existingIds = existing.map((n) => n.nodeId);
  
    // Step 3: Collect missing ones
    const missingIds = nodeIds.filter((nodeId) => !existingIds.includes(nodeId));
    if (missingIds.length) {
      throw new ApiErrorResponse(StatusCodes.BAD_REQUEST, `Nodes not found: ${missingIds.join(", ")}`);
    }
  
    return existingIds; // normalized valid list
  } catch (error:any) {
    if(error instanceof ApiErrorResponse){
      throw error;
    }
    throw new ApiErrorResponse(StatusCodes.INTERNAL_SERVER_ERROR, error.message);
  }
}

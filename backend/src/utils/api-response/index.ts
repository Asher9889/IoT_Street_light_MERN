import ApiErrorResponse from "./ApiErrorResponse";
import ApiSuccessResponse from "./ApiSuccessResponse";


export { ApiErrorResponse, ApiSuccessResponse }


// Api Messages


import { nodeApiMessage, gatewayApiMessage } from "./devices/device.apiMessages";


export const deviceApiMessage = { 
    nodeApiMessage,
    gatewayApiMessage
}




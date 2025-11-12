import { Gateway, GatewayLog } from "../../../models";
import { GatewayMessageType } from "../../interfaces";
import { EventLogsLevelEnum } from "../../../models/gatewayLogs.model";

interface IGatewayTelemetry {
    type: string;
    deviceId: string;
    gatewayId: string;
    uptime_s: number;
    nodeCount: number;
}

export default async function handleGatewayStatus(topic: string, message: Buffer) {
  try {
    if(!message) return;
    const payload:IGatewayTelemetry = JSON.parse(message.toString());
    console.log("[GATEWAY] Received telemetry:", payload);
    const { deviceId, gatewayId, uptime_s, nodeCount } = payload;
    
    const gateway = await Gateway.findOne({ macAddress: deviceId }).lean();
    if(!gateway) return;
  
    if(gatewayId !== gateway.gatewayId){
        console.log("[GATEWAY] Gateway ID mismatch for telemetry:", gatewayId, gateway.gatewayId);
        return;
    }
    await GatewayLog.create({
      gatewayId: gateway.gatewayId,
      level: "info", 
      event: GatewayMessageType.DEVICE_TELEMETRY,
      message: "Gateway sends new telemetry",
      payload: {
        deviceId,
        uptime_s,
        nodeCount
      },
      timestamp: new Date(),
    })
    console.log(`[GATEWAY] Telemetry received for ${gateway.gatewayId}`);
  } catch (error) {
    console.error("[GATEWAY] Error handling telemetry:", error);
  }
}

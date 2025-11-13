export enum GatewayMessageType {
  DEVICE_REGISTER = "device_register",
  DEVICE_CONFIG = "device_config",
  HEARTBEAT = "heartbeat",
  SENSOR_DATA = "sensor_data",
  CONFIG_ACK = "config_ack",
  DEVICE_OFFLINE = "OFFLINE",
  DEVICE_TELEMETRY = "telemetry",
  NODE_REGISTER = "node_register",
  NODE_CONTROL = "node_control",
}

export interface IGatewayBase {
  deviceId: string;
  firmwareVersion: string;
}

export type IGatewayOfflineMessage = {
  type: GatewayMessageType.DEVICE_OFFLINE;
}
export interface IGatewayRegisterMessage extends IGatewayBase {
  type: GatewayMessageType.DEVICE_REGISTER;
}

export interface IGatewayHeartbeatMessage extends IGatewayBase {
  type: GatewayMessageType.HEARTBEAT;
}
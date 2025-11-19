export const STATUS = {
  ONLINE: "ONLINE",
  OFFLINE: "OFFLINE",
} as const;


export const MODE = {
  AUTO: "AUTO",
  MANUAL: "MANUAL",
} as const;

export const LIGHT_STATE = {
  ON: "ON",
  OFF: "OFF",
  AUTO: "AUTO",
} as const;

export const COMMAND_STATUS = {
  PENDING: "PENDING",
  ACKED: "ACKED",
  FAILED: "FAILED",
  EXPIRED: "EXPIRED",
} as const;

export type Status = (typeof STATUS)[keyof typeof STATUS];
export type Mode = (typeof MODE)[keyof typeof MODE];
export type TLightState = (typeof LIGHT_STATE)[keyof typeof LIGHT_STATE];
export type TCommandStatus = (typeof COMMAND_STATUS)[keyof typeof COMMAND_STATUS];

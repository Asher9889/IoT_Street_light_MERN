export const STATUS = {
  ONLINE: "ONLINE",
  OFFLINE: "OFFLINE",
} as const;

export type Status = (typeof STATUS)[keyof typeof STATUS];

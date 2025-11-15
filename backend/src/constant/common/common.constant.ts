export const STATUS = {
  ONLINE: "ONLINE",
  OFFLINE: "OFFLINE",
} as const;

export type Status = (typeof STATUS)[keyof typeof STATUS];

export const MODE = {
  AUTO: "AUTO",
  MANUAL: "MANUAL",
} as const;

export type Mode = (typeof MODE)[keyof typeof MODE];

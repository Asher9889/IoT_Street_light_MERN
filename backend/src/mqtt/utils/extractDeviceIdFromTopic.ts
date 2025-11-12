/**
 * Extracts deviceId or gatewayId from an MQTT topic.
 * Automatically detects based on known ID prefixes.
 *
 * Examples:
 *  - iot/gateway/deviceC0964CBBBC2C/register       → deviceC0964CBBBC2C
 *  - iot/gateway/GW-4/node/register                → GW-4
 *  - iot/gateway/GW-4/node/node123/config/set      → GW-4
 */
export function extractIdFromTopic(topic: string): string | null {
  if (!topic) return null;
  const parts = topic.split("/");

  // Fast path: find the first part that starts with known prefixes
  for (const part of parts) {
    if (part.startsWith("device")) return part; // deviceId
    if (part.startsWith("GW")) return part;     // gatewayId
  }

  return null;
}

/**
 * Specifically extract the deviceId (starts with "device").
 */
export function extractDeviceIdFromTopic(topic: string): string | null {
  if (!topic) return null;
  const match = topic.match(/device[0-9A-Fa-f]+/);
  return match ? match[0] : null;
}

/**
 * Specifically extract the gatewayId (starts with "GW").
 */
export function extractGatewayIdFromTopic(topic: string): string | null {
  if (!topic) return null;
  const match = topic.match(/GW[\w-]*/);
  return match ? match[0] : null;
}


/**
 * Specifically extract the nodeId (starts with "node").
 */
export function extractNodeIdFromTopic(topic: string): string | null {
  if (!topic) return null;
  const match = topic.match(/node[0-9A-Fa-f]+/);
  return match ? match[0] : null;
}

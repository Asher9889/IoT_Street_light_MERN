export default function identifyTopicType(topic: string) {
  const parts = topic.split("/");

  if (parts[0] !== "iot" || parts[1] !== "gateway") {
    return {
      isGatewayRegister: false,
      isGatewayStatus: false,
      isNodeRegister: false,
      isNodeConfigAck: false,
      isNodeControlAck: false
    };
  }

  const action = parts[parts.length - 1];
  const subAction = parts[parts.length - 2];

  return {
    // iot/gateway/:gw/register
    isGatewayRegister: parts.length === 4 && action === "register",

    // iot/gateway/:gw/status
    isGatewayStatus: parts.length === 4 && action === "status",

    // iot/gateway/:gw/node/:nodeId/register
    isNodeRegister: parts.length === 6 && parts[3] === "node" && action === "register",

    // iot/gateway/:gw/node/:nodeId/config/ack
    isNodeConfigAck: parts.length === 7 && parts[3] === "node" && subAction === "config" && action === "ack",

    // iot/gateway/:gw/node/:nodeId/control/ack
    isNodeControlAck: parts.length === 7 && parts[3] === "node" && subAction === "control" && action === "ack"
  };
}

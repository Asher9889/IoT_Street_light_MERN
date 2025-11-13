import { initMQTTClient, getMQTTClient } from "./client";
import { subscribeGatewayTopics } from "./topics";
import handleGatewayRegistration from "./handlers/gateway/handleGatewayRegistration";
import handleGatewayConfigSet from "./handlers/gateway/handleGatewayConfigSet";
import handleGatewayStatus from "./handlers/gateway/handleGatewayStatus";
import handleGatewayBootstrapConfig from "./handlers/gateway/handleGatewayBootstrapConfig"; 
import handleNodeAck from "./handlers/node/handleNodeAck";
import { controlNode } from "./handlers/node/controlNode";

import { extractDeviceIdFromTopic, extractGatewayIdFromTopic, extractNodeIdFromTopic } from "./utils/extractDeviceIdFromTopic";


export { handleNodeAck, extractDeviceIdFromTopic, extractGatewayIdFromTopic, extractNodeIdFromTopic, handleGatewayBootstrapConfig, initMQTTClient, getMQTTClient, subscribeGatewayTopics, handleGatewayRegistration, handleGatewayConfigSet, handleGatewayStatus }

export const mqttService = {
    controlNode,
}

// Interfaces

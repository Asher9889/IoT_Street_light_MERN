import { initMQTTClient, getMQTTClient } from "./client";
import { subscribeGatewayTopics } from "./topics";
import handleGatewayRegistration from "./handlers/gateway/handleGatewayRegistration";
import handleGatewayConfigSet from "./handlers/gateway/handleGatewayConfigSet";
import handleGatewayStatus from "./handlers/gateway/handleGatewayStatus";
import handleGatewayBootstrapConfig from "./handlers/gateway/handleGatewayBootstrapConfig"; 
import handleNodeAck from "./handlers/node/handleNodeAck";
import { controlNode } from "./handlers/node/controlNode";
import handleNodeControlAck from "./handlers/node/handleNodeControlAck";
import identifyTopicType from "./utils/identifyTopicType";
import { extractDeviceIdFromTopic, extractGatewayIdFromTopic, extractNodeIdFromTopic } from "./utils/extractDeviceIdFromTopic";


export { identifyTopicType, handleNodeControlAck, handleNodeAck, extractDeviceIdFromTopic, extractGatewayIdFromTopic, extractNodeIdFromTopic, handleGatewayBootstrapConfig, initMQTTClient, getMQTTClient, subscribeGatewayTopics, handleGatewayRegistration, handleGatewayConfigSet, handleGatewayStatus }

export const mqttService = {
    controlNode,
}

// Interfaces

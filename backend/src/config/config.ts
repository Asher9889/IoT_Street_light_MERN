import dotenv from "dotenv"

dotenv.config();

type Config = {
    port: number;
    mongoDBURL: string;
    dbName: string;
    mqtt_client: string;
    mqtt_port: string;
}




const config: Config = {
    port: Number(process.env.PORT),
    mongoDBURL: String(process.env.MONGODB_URL),
    dbName: String(process.env.MONGO_DB_NAME),
    mqtt_client: String(process.env.MQTT_CLIENT),
    mqtt_port: String(process.env.MQTT_PORT),
}

const mqttBrokerUrl = `${config.mqtt_client}:${config.mqtt_port}`;


const devicesConstant = {
    MAX_GATEWAY_LIMIT: Number(process.env.MAX_GATEWAY_LIMIT),
    MAX_NODE_LIMIT: Number(process.env.MAX_NODE_LIMIT),
}


export { config, devicesConstant, mqttBrokerUrl };

import mqtt from 'mqtt';

const mqttBrokerUrl = 'mqtt://broker.hivemq.com';
const client = mqtt.connect(mqttBrokerUrl);

// Handle connection
client.on('connect', () => {
  console.log('✅ MQTT connected to broker:', mqttBrokerUrl);
});

// Handle errors
client.on('error', (err) => {
  console.error('❌ MQTT error:', err);
});

// Dynamic publisher
function publishBulbStates(deviceId: string, bulbStates: string[]) {
  const topic = `iot/devices/${deviceId}/cmd`;
  const payload = JSON.stringify({
    status: bulbStates,
    timestamp: new Date().toISOString()
  });

  client.publish(topic, payload, {}, (err) => {
    if (err) {
      console.error(`❌ Failed to publish to ${topic}:`, err);
    } else {
      console.log(`📤 Published to ${topic}:`, payload);
    }
  });

  
}

function publishSingleBulb(deviceId: string, index: number, status: string) {
  const topic = `iot/devices/${deviceId}/cmd`;
  const payload = JSON.stringify({
    index,
    status,
    timestamp: new Date().toISOString()
  });

  client.publish(topic, payload, {}, (err) => {
    if (err) {
      console.error(`❌ Failed to publish to ${topic}:`, err);
    } else {
      console.log(`📤 Published single bulb to ${topic}:`, payload);
    }
  });
}

export { publishBulbStates, publishSingleBulb};
import mqtt from 'mqtt';

const mqttBrokerUrl = 'mqtt://broker.hivemq.com';
const topic = 'home/relays';


const client = mqtt.connect(mqttBrokerUrl); // connect to broker

client.on('connect', () => {
  console.log('✅ MQTT connected');
});

client.on('error', (err) => {
  console.error('❌ MQTT error:', err);
});

function publishBulbStates(bulbStates: string[]) {
  const payload = JSON.stringify(bulbStates);
  client.publish(topic, payload, {}, (err) => {
    if (err) {
      console.error('❌ Failed to publish MQTT:', err);
    } else {
      console.log('📤 MQTT published:', payload);
    }
  });
}

export default publishBulbStates;

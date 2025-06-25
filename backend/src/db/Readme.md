## 🧱 MongoDB Schema Design

- 📄 devices Collection
```
{
  _id: ObjectId,
  deviceId: "device001",
  simId: "AIRTEL-1234567890",         // optional tracking SIM
  location: {
    name: "Office 1",
    lat: 28.612,                      // optional
    lon: 77.229
  },
  relayStates: [                     // current relay states
    "ON", "OFF", "OFF", ..., "OFF"
  ],
  lastSeen: ISODate("2025-06-25T13:00:00Z"), // last MQTT ping
  description: "Main Hall controller"
}
```
---

## 📡 MQTT Topics Design 
| Topic Type           | Topic Example                    | Payload Example             |
| -------------------- | -------------------------------- | --------------------------- |
| **Subscribe (CMD)**  | `iot/devices/device001/cmd`      | `["ON", "OFF", ..., "OFF"]` |
| **Publish (Status)** | `iot/devices/device001/status`   | `["ON", "OFF", ..., "OFF"]` |
| **LWT (Last Will)**  | `iot/devices/device001/lastseen` | `{"status": "offline"}`     |

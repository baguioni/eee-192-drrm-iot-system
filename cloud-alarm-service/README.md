# Cloud Alarm Service

This uses Firebase Cloud Functions integrated with the Twilio API. Once a valid request is made, an SMS to the predefined phone numbers will be sent.

## Prerequisites

- Node.js
- Firebase CLI
- Twilio Account (with Account SID, Auth Token, and Phone Number)
- Firebase Project (with Firebase Functions enabled)

## Example Request Body

To send a sensor data request to the function, provide the following JSON structure in the body of an HTTP POST request:

```json
{
  "sensorType": 0,
  "value": 25.5
}
```

the `sensorType` field can take the following values to represent the sensor.

| Sensor Type  | Value |
|--------------|-------|
| Voltage      | 0     |
| Humidity     | 1     |
| Temperature  | 2     |
| Smoke        | 3     |
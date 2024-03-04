const { onRequest } = require("firebase-functions/v2/https");
const { logger } = require("firebase-functions");

const twilio = require("twilio");

const SensorType = {
    Voltage: 0,
    Humidity: 1,
    Temperature: 2,
    Smoke: 3
};

class TwilioService {
    constructor() {
        this.accountSid = process.env.TWILIO_ACCOUNT_SID;
        this.authToken = process.env.TWILIO_AUTH_TOKEN;
        this.twilioPhoneNumber = process.env.TWILIO_PHONE_NUMBER;

        if (!this.accountSid || !this.authToken || !this.twilioPhoneNumber) {
            throw new Error("Twilio credentials are missing");
        }

        this.client = twilio(this.accountSid, this.authToken);
    }

    async sendSMS(to, body) {
        await this.client.messages.create({
            body,
            from: this.twilioPhoneNumber,
            to
        });
    }

    async sendSensorData(to, sensorData) {
        const messageBody = `Sensor Alert - ${Object.keys(SensorType)[sensorData.sensorType]}:\nValue: ${sensorData.value}`;
        await this.sendSMS(to, messageBody);
    }
}

// Function logic
exports.sensorAlert = onRequest(async (request, response) => {
    const { sensorType, value } = request.body;

    logger.info("Received sensor data", { sensorType, value });
    const twilioService = new TwilioService();

    try {
        await twilioService.sendSensorData("+1234567890", {
            sensorType: SensorType.Voltage,
            value: 5
        });
        response.status(200).send("Sensor data received successfully");
    } catch (error) {
        logger.error("Error sending SMS", error);
        response.status(500).send("Error sending SMS");
    }
});
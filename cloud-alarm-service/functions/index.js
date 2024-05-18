const { onRequest } = require("firebase-functions/v2/https");
const { logger } = require("firebase-functions");

const twilio = require("twilio");
const TelegramBot = require("node-telegram-bot-api");

const SensorType = {
  Voltage: 0,
  Humidity: 1,
  Temperature: 2,
  Smoke: 3,
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
      to,
    });
  }

  async sendSensorData(to, sensorData) {
    let messageBody;

    if (!sensorData.value) {
      messageBody = `Sensor Alert - ${
        Object.keys(SensorType)[sensorData.sensorType]
      }`;
    } else {
      messageBody = `Sensor Alert - ${
        Object.keys(SensorType)[sensorData.sensorType]
      }\nValue: ${sensorData.value}`;
    }

    await this.sendSMS(to, messageBody);
  }
}

class TelegramService {
  constructor() {
    this.apiKey = process.env.TELEGRAM_BOT_API_KEY;

    if (!this.apiKey) {
      throw new Error("Telegram credentials are missing");
    }

    this.client = new TelegramBot(this.apiKey, { polling: true });
  }

  async sendMessage(to, body) {
    await this.client.sendMessage(to, body);
  }

  async sendSensorData(to, sensorData) {
    let messageBody;

    if (!sensorData.value) {
      messageBody = `Sensor Alert - ${
        Object.keys(SensorType)[sensorData.sensorType]
      }`;
    } else {
      messageBody = `Sensor Alert - ${
        Object.keys(SensorType)[sensorData.sensorType]
      }\nValue: ${sensorData.value}`;
    }

    await this.sendMessage(to, messageBody);
  }
}

// Function logic
// Activate twilio during actual deployment
exports.sensorAlert = onRequest(async (request, response) => {
  const { sensorType, value } = request.body;

  // value is optional
  if (!sensorType) {
    logger.error("Request is missing sensor type");
    response.status(400).send("Request is missing sensor type");
    return;
  }

  logger.info("Received sensor data", { sensorType, value });
  // const twilioService = new TwilioService();
  const telegramService = new TelegramService();

  try {
    // await twilioService.sendSensorData(process.env.TWILIO_RECIPIENT_PHONE_NUMBER, {
    //     sensorType: sensorType,
    //     value: value
    // });

    await telegramService.sendSensorData(process.env.TELEGRAM_CHAT_ID, {
      sensorType: sensorType,
      value: value,
    });

    response.status(200).send("Sensor data received successfully");
  } catch (error) {
    logger.error("Error sending notifications", error);
    response.status(500).send("Error sending notifications");
  }
});

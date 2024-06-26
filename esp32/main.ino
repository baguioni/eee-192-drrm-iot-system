#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_system.h"
#include "driver/uart.h"
#include "string.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define UART_BAUD_RATE 115200
#define SENSOR_THRESHOLD 300

const char *ssid = "";
const char *password = "";

unsigned long timerDelay = 5000;
unsigned long lastTime = 0;

String thingspeakEndpoint = "https://api.thingspeak.com/update?api_key=";
String alertEndpoint = "https://us-central1-cloud-alarm-service.cloudfunctions.net/sensorAlert";
String apiKey = "CFBV2ROKPP58IHBL";

/*
thingspeak fields

    VOLTAGE - 1
    HUMIDITY - 2
    TEMPERATURE -3 
    SMOKE- 4
*/

/*
cloud alarm fields

    VOLTAGE - 0
    HUMIDITY - 1
    TEMPERATURE - 2 
    SMOKE - 3
*/

int parseJson(const char *jsonString, String &sensorTypeString, String &valueString)
{
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error)
    {
        int sensorType = doc["sensorType"];
        int value = doc["value"];

        // Convert integers to strings
        sensorTypeString = String(sensorType + 1); // Plus 1 to match field names in thingspeak;
        valueString = String(value);
        return 1;
    }
    else
    {
        Serial.println("Failed to parse JSON");
        return 0;
    }
}

int  parseDHTJson(const char *jsonString, String &tempValueString, String &humValueString)
{
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error)
    {
        int tempValue = doc["temp_value"];
        int humValue = doc["hum_value"];

        // Convert integers to strings
        tempValueString = String(tempValue);
        humValueString = String(humValue);

        return 1;
    }
    else
    {
        Serial.println("Failed to parse JSON");
        return 0;
    }
}

void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void initUart()
{
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, 18, 19));
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size,
                                        uart_buffer_size, 10, &uart_queue, 0));
    Serial.println("UART2 Connected");
}

int sendDataToThinkSpeak(const String &endpoint, const String &apiKey, const String &sensorTypeString, const String &valueString)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String serverPath = endpoint + apiKey + "&field" + sensorTypeString + "=" + valueString;
        http.begin(serverPath.c_str());
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            return httpResponseCode;
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            return httpResponseCode;
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
    return 0;
}

void sendDataToSensorAlert(const String &endpoint, const String &sensorTypeString, const String &valueString)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(endpoint.c_str());
        
        // Specify content-type header
        http.addHeader("Content-Type", "application/json");
        
        // Create JSON object
        StaticJsonDocument<200> jsonDoc;
        jsonDoc["sensorType"] = sensorTypeString;
        jsonDoc["value"] = valueString;
        
        // Serialize JSON to string
        String requestBody;
        serializeJson(jsonDoc, requestBody);
        
        // Send POST request
        int httpResponseCode = http.POST(requestBody);
        
        if (httpResponseCode > 0)
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}

void setup()
{
    Serial.begin(115200);
    initWiFi();
    initUart();

    sendWifiStatus();
    
    Serial.print("RRSI: ");
    Serial.println(WiFi.RSSI());
}

void sendWifiStatus() {
      const uart_port_t uart_num = UART_NUM_2;
      if (WiFi.status() == WL_CONNECTED) {
        uart_write_bytes_with_break(uart_num, "C",strlen("D"), 100);
        Serial.println("WiFi Status: Connected");
      } else {
        uart_write_bytes_with_break(uart_num, "D",strlen("D"), 100);
        Serial.println("WiFi Status: Disconnected");
      }
}

void loop()
{
    const uart_port_t uart_num = UART_NUM_2;
    sendWifiStatus();
    uint8_t uart_data[128];
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t *)&length));
    length = uart_read_bytes(uart_num, uart_data, sizeof(uart_data), 100);
    if (length > 0)
    {
        char *jsonString = (char *)uart_data;
        Serial.println(jsonString);
        String sensorTypeString;
        String valueString;
        int successful_parse = parseJson(jsonString, sensorTypeString, valueString);
        
        if(successful_parse) {
          Serial.println("Sensor Type: SMOKE");
          Serial.println("Sensor Data: "+ valueString);
          int result = sendDataToThinkSpeak(thingspeakEndpoint, apiKey, sensorTypeString, valueString);
          // sendWifiStatus();
          // Reset if error occurs
          if (result != 200) {
            // Write data to UART, end with a break signal.
            uart_write_bytes_with_break(uart_num, "D",strlen("D"), 100);
            Serial.println("WiFi Status: Disconnected");
            ESP.restart();
          }
          uart_write_bytes_with_break(uart_num, "C",strlen("D"), 100);
          Serial.println("WiFi Status: Connected");

          //Send to cloud function since alerts are limited
          if (valueString.toInt() >= SENSOR_THRESHOLD) {
            sendDataToSensorAlert(alertEndpoint, String(sensorTypeString.toInt() - 1), valueString);
          }
        }

        /* Uncomment if DHTSensor
        String tempValueString;
        String humValueString;
        int successful_parse = parseDHTJson(jsonString, tempValueString, humValueString);
        if(successful_parse) {
          Serial.println(tempValueString);
          Serial.println(humValueString);
          int result_1 = sendDataToThinkSpeak(thingspeakEndpoint, apiKey, "3", tempValueString);
          int result_2 = sendDataToThinkSpeak(thingspeakEndpoint, apiKey, "2", humValueString);
          // sendWifiStatus();
          // Reset if error occurs
          if (result_1 != 200) {
            uart_write_bytes_with_break(uart_num, "D",strlen("D"), 100);
            Serial.println("WiFi Status: Disconnected");
            ESP.restart();

          }

          if (result_2 != 200) {
            uart_write_bytes_with_break(uart_num, "D",strlen("D"), 100);
            Serial.println("WiFi Status: Disconnected");
            ESP.restart();
          }

          uart_write_bytes_with_break(uart_num, "C",strlen("D"), 100);
          Serial.println("WiFi Status: Connected");

          //Send to cloud function since alerts are limited
          if (tempValueString.toInt() >= SENSOR_THRESHOLD) { // change value as needed
            sendDataToSensorAlert(alertEndpoint, "2", tempValueString);
          }

          if (humValueString.toInt() >= SENSOR_THRESHOLD) { // change value as needed
            sendDataToSensorAlert(alertEndpoint, "1", humValueString);
          }
        }
        */
    }
}

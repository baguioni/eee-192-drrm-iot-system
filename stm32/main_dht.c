#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// stm specific headers
#include <stm32f4xx.h>
#include <stm32f411xe.h>

// custom headers
#include "usart.h"
#include "message_queue.h"
#include "dht11.h"
#include "delay.h"
#include "json.h"

// STM32 specific values
// Change as needed. Refer to the message_queue.h file
#define SENSORTYPE SMOKE
#define SENSORDELAY 1000

int main(void) {
	/*
		Initializes usart6 as well

		usart6 - esp32
		usart2 - pc for debugging
	*/
	usart2_init();
	dht11_PinA8_Init();

	usart2_tx_send("STM32 starting operations ...", sizeof("STM32 starting operations ..."));

	float temperature_value, humidity_value;
	float dequeued_temperature, dequeued_humidity;

    // Initialize a queue
	Queue* temperature_queue = createQueue();
	Queue* humidity_queue = createQueue();

	/* Loop forever */
	while (1) {
		dht11_start();

		bool wifiModuleConnected = getWifiStatus();

		if (wifiModuleConnected && !isEmpty(temperature_queue)) {
			// Remove element in queue since it was already sent
			dequeue(temperature_queue, &dequeued_temperature);
		}

		if (wifiModuleConnected && !isEmpty(humidity_queue)) {
			// Remove element in queue since it was already sent
			dequeue(humidity_queue, &dequeued_humidity);
		}

		if (Check_Response() == Response_OK) {
			Get_DHT_Data(&temperature_value, &humidity_value);

			enqueue(temperature_queue, temperature_value);
			enqueue(humidity_queue, humidity_value);
		}
		
		// Send to serial monitor
		if (!usart6_tx_is_busy() && !isEmpty(temperature_queue)) {
			float temperature_dequeue_result = peek(temperature_queue);
			char *json_temp = createJSONString(temperature_dequeue_result, TEMPERATURE);
			usart6_tx_send(json_temp, strlen(json_temp));
			usart2_tx_send(json_temp, strlen(json_temp));

			// Free dynamically allocated memory
			free(json_temp);
		}

		if (!usart6_tx_is_busy() && !isEmpty(humidity_queue)) {
			float humidity_dequeue_result = peek(humidity_queue);

			char *json_hum = createJSONString(humidity_dequeue_result, HUMIDITY);
			usart6_tx_send(json_hum, strlen(json_hum));
			usart2_tx_send(json_hum, strlen(json_hum));

			// Free dynamically allocated memory
			free(json_hum);
		}

		delay(SENSORDELAY);
	}
}

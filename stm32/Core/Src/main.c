#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// stm specific headers
#include <stm32f4xx.h>
#include <stm32f411xe.h>

// custom headers
#include "adc.h"
#include "usart.h"
#include "message_queue.h"

// STM32 specific values
// Change as needed. Refer to the message_queue.h file
#define SENSORTYPE SMOKE
#define SENSORDELAY 1000

char* createJSONString(uint16_t value);

char* createJSONString(uint16_t value) {
    // Calculate the length of the JSON string
    size_t jsonLength = 50; // Initial length for JSON formatting

    // Allocate memory for the JSON string
    char* jsonString = (char*)malloc(jsonLength * sizeof(char));
    if (jsonString == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Construct the JSON string
    snprintf(jsonString, jsonLength, "{\"sensorType\":%d,\"value\":%d}", SENSORTYPE, value);

    return jsonString;
}

uint16_t adc_value = 0x0000;
uint16_t dequeued_value;

int main(void) {
	usart2_init();
	ADC_init();

    // Initialize a queue
	Queue* queue = createQueue();

	/* Loop forever */
	while (1) {
		delay_ms(SENSORDELAY);

		// ADC related functionality
		ADC_enable();
		ADC_startconv();
		ADC_waitconv();
		adc_value = ADC_GetVal();

		// Message Queue related functionality
		enqueue(queue, adc_value);

		// Send to serial monitor
	    if (!usart2_tx_is_busy() && !isEmpty(queue)) {
	    	DequeueResult result = dequeue(queue, &dequeued_value);

	        if (result == DequeueResult_Success) {
		    	char* jsonDatapoint = createJSONString(dequeued_value);
				usart2_tx_send(jsonDatapoint, strlen(jsonDatapoint));
			    // Free dynamically allocated memory
			    free(jsonDatapoint);
	        }
	    }
	}
}


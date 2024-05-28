#include <stdint.h>
#include <string.h>

char *createJSONString(int value, uint16_t type) {
	// Calculate the length of the JSON string
	size_t jsonLength = 50; // Initial length for JSON formatting

	// Allocate memory for the JSON string
	char *jsonString = (char *)malloc(jsonLength * sizeof(char));

	// Construct the JSON string
	snprintf(jsonString, jsonLength, "{\"sensorType\":%d,\"value\":%d}", type, value);

	return jsonString;
}

char *createDHTJSONString(int temp_value, int hum_value) {
	// Calculate the length of the JSON string
	size_t jsonLength = 50; // Initial length for JSON formatting

	// Allocate memory for the JSON string
	char *jsonString = (char *)malloc(jsonLength * sizeof(char));

	// Construct the JSON string
	snprintf(jsonString, jsonLength, "{\"temp_value\":%d, \"hum_value\":%d}", temp_value, hum_value);

	return jsonString;
}

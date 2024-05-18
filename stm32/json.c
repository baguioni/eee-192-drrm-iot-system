#include <stdint.h>
#include <string.h>

char *createJSONString(float value, uint16_t type) {
    size_t jsonLength = 50;

    char *jsonString = (char *)malloc((jsonLength + 1) * sizeof(char));

    if (jsonString == NULL) {
        return NULL;
    }

    int charsWritten = snprintf(jsonString, jsonLength + 1, "{\"sensorType\":%d,\"value\":%.2f}", type, value);

    if (charsWritten < 0 || charsWritten > jsonLength) {
        free(jsonString);
        return NULL;
    }

    return jsonString;
}
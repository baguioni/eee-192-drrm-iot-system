#ifndef JSON_CREATOR_H
#define JSON_CREATOR_H

#include <stdint.h>

// Function declaration
char *createJSONString(int value, uint16_t type);
char *createDHTJSONString(int temp_value, int hum_value)

#endif /* JSON_CREATOR_H */

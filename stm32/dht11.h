#ifndef DHT11_H_
#define DHT11_H_

#include "stdint.h"

typedef enum {
	Response_OK=0,
	Response_ERR=1

} DHT11_Response_Typedef;

void dht11_PinA8_Init(void);
void dht11_start(void);
DHT11_Response_Typedef Check_Response();
void Get_DHT_Data(float * TEMP, float *RH);

#endif
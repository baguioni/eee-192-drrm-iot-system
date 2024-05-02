/*
 * adc.h
 *
 *  Created on: May 3, 2024
 *      Author: baguioni
 */

#ifndef SRC_ADC_H_
#define SRC_ADC_H_

void ADC_init(void);
void ADC_enable(void);
void ADC_startconv(void);
void ADC_waitconv(void);
int ADC_GetVal(void);
void delay_ms(int delay);

#endif /* SRC_ADC_H_ */

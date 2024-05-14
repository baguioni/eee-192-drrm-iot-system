#include <stdint.h>
#include <stm32f4xx.h>
#include <stm32f411xe.h>
#include "adc.h"

// Pin PA1
void ADC_init(void) {
	//enable adc clock
	RCC->APB2ENR |= (1 << 8);

	//prescaler = 2
	ADC->CCR &= ~(1 << 16);
	ADC->CCR &= ~(1 << 17);

	//configure ADC resolution
	ADC1->CR1 &= ~(1 << 25);
	ADC1->CR1 &= ~(1 << 24);

	//Configure to Scan mode
	ADC1->CR1 |= (1 << 8);
	//Enable Interrupt for EOC
	ADC1->CR1 |= (1 << 5);
	//configure sampling time
	ADC1->SMPR2 &= ~(1 << 5);
	ADC1->SMPR2 &= ~(1 << 4);
	ADC1->SMPR2 |= (1 << 3);
	//end of conversion selection
	ADC1->CR2 &= ~(1 << 10);

	//configure data alignment
	ADC1->CR2 &= ~(1 << 11);

	//total number of conversions in the channel conversion sequence
	ADC1->SQR1 &= ~(1 << 23);
	ADC1->SQR1 &= ~(1 << 22);
	ADC1->SQR1 &= ~(1 << 21);
	ADC1->SQR1 &= ~(1 << 20);

	//assign channel for first conversion
	ADC1->SQR3 |= (1 << 0);

	//cont conversion mode
	ADC1->CR2 |= (1 << 1);
}

void ADC_enable(void) {
	ADC1->CR2 |= (1 << 0); //enable the adc
	delay_ms(1); // required to ensure adc stable
}

void ADC_startconv(void) {
	ADC1->CR2 |= (1 << 30);
}

void ADC_waitconv(void) {
	//wait for the end of conversion
	while (!((ADC1->SR) & (1 << 1))) {
		;
	}
}

int ADC_GetVal(void) {
	return ADC1->DR; //read the value contained at the data register
}

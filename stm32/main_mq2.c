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
#include "delay.h"
#include "json.h"

// STM32 specific values
// Change as needed. Refer to the message_queue.h file
#define SENSORTYPE SMOKE
#define SENSORDELAY 5000 // 30 seconds


// Function to initialize the system; called only once on device reset
static void do_sys_config(void)
{
	// See the top of this file for assumptions used in this initialization.


	////////////////////////////////////////////////////////////////////

	RCC->AHB1ENR |= (1 << 0);	// Enable GPIOA

	GPIOA->MODER &= ~(0b11 << 10);	// Set PA5 as input...
	GPIOA->MODER |=  (0b10 << 10);	// ... then set it as alternate function.
	GPIOA->OTYPER &= ~(1 << 5);	// PA5 = push-pull output
	GPIOA->OSPEEDR &= ~(0b11 << 10);	// Fast mode (needed for PWM)
	GPIOA->OSPEEDR |=  (0b10 << 10);

	/*
	 * For PA5 -- where the LED on the Nucleo board is -- only TIM2_CH1
	 * is PWM-capable, per the *device* datasheet. This corresponds to
	 * AF01.
	 */
	GPIOA->AFR[0] &= ~(0x00F00000);	// TIM2_CH1 on PA5 is AF01
	GPIOA->AFR[0] |=  (0x00100000);

	////////////////////////////////////////////////////////////////////

	/*
	 * In this setup, TIM2 is used for brightness control (see reason
	 * above), while TIM1 is used to do input-capture on PA9 via
	 * Channel 2.
	 *
	 * The internal clock for TIM2 is the same as the APB1 clock;
	 * in turn, this clock is assumed the same as the system (AHB) clock
	 * without prescaling, which results to the same frequency as HSI
	 * (~16 MHz).
	 *
	 * TIM1, on the other hand, uses the APB2 clock; in turn, no prescaling
	 * is done here, which also yields the same value as HSI (~16 MHz).
	 */
	RCC->APB1ENR	|= (1 << 0);	// Enable TIM2
	RCC->APB2ENR	|= (1 << 0);	// Enable TIM1

	/*
	 * Classic PWM corresponds to the following:
	 * 	- Edge-aligned	(CMS  = CR1[6:5] = 0b00)
	 * 	- Upcounting	(DIR  = CR1[4:4] = 0)
	 *	- Repetitive	(OPM  = CR1[3:3] = 0)
	 * 	- PWM Mode #1	(CCxS[1:0] = 0b00, OCMx = 0b110)
	 * 		- These are in CCMRy; y=1 for CH1 & CH2, and y=2 for
	 * 		  CH3 & CH4.
	 */
	TIM2->CR1 &= ~(0b1111 << 0);
	TIM2->CR1 &= ~(1 << 0);		// Make sure the timer is off
	TIM2->CR1 |=  (1 << 7);		// Preload ARR (required for PWM)
	TIM2->CCMR1 = 0x0068;		// Channel 1 (TIM2_CH1)

	/*
	 * Per the Nyquist sampling theorem (from EEE 147), to appear
	 * continuous the PWM frequency must be more than twice the sampling
	 * frequency. The human eye (the sampler) can usually go up to 24Hz;
	 * some, up to 40-60 Hz. To cover all bases, let's use 500 Hz.
	 *
	 * In PWM mode, the effective frequency changes to
	 *
	 * (f_clk) / (ARR*(PSC+1))
	 *
	 * since each period must be able to span all values on the interval
	 * [0, ARR). For obvious reasons, ARR must be at least equal to one.
	 */
	TIM2->ARR	= 100;		// Integer percentages; interval = [0,100]
	TIM2->PSC	= (320 - 1);	// (16MHz) / (ARR*(TIM2_PSC + 1)) = 500 Hz

	// Let main() set the duty cycle. We initialize at zero.
	TIM2->CCR1	= 0;

	/*
	 * The LED is active-HI; thus, its polarity bit must be cleared. Also,
	 * the OCEN bit must be enabled to actually output the PWM signal onto
	 * the port pin.
	 */
	TIM2->CCER	= 0x0001;

	////////////////////////////////////////////////////////////////////

	// Pushbutton configuration
	RCC->AHB1ENR |= (1 << 2);	// Enable GPIOC

	GPIOC->MODER &= ~(0b11 << 26);	// Set PC13 as input...
	GPIOC->PUPDR &= ~(0b11 << 26);	// ... without any pull-up/pull-down (provided externally).

	////////////////////////////////////////////////////////////////////

	/*
	 * Enable the system-configuration controller. If disabled, interrupt
	 * settings cannot be configured.
	 */
	RCC->APB2ENR |= (1 << 14);

	/*
	 * SYSCFG_EXTICR is a set of 4 registers, each controlling 4 external-
	 * interrupt lines. Within each register, 4 bits are used to select
	 * the source connected to each line:
	 * 	0b0000 = Port A
	 * 	0b0001 = Port B
	 * 	...
	 * 	0b0111 = Port H
	 *
	 * For the first EXTICR register:
	 *	Bits 0-3   correspond to Line 0;
	 * 	Bits 4-7   correspond to Line 1;
	 *	Bits 8-11  correspond to Line 2; and
	 * 	Bits 12-15 correspond to Line 3.
	 *
	 * The 2nd EXTICR is for Lines 4-7; and so on. Also, the line numbers
	 * map 1-to-1 to the corresponding bit in each port. Thus, for example,
	 * a setting of EXTICR2[11:8] == 0b0011 causes PD6 to be tied to
	 * Line 6.
	 *
	 * For this system, PC13 would end up on Line 13; thus, the
	 * corresponding setting is EXTICR4[7:4] == 0b0010. Before we set it,
	 * mask the corresponding interrupt line.
	 */
	EXTI->IMR &= ~(1 << 13);		// Mask the interrupt
	SYSCFG->EXTICR[3] &= (0b1111 << 4);	// Select PC13 for Line 13
	SYSCFG->EXTICR[3] |= (0b0010 << 4);

	/*
	 * Per the hardware configuration, pressing the button causes a
	 * falling-edge event to be triggered, and a rising-edge on release.
	 * Since we are only concerned with presses, just don't trigger on
	 * releases.
	 */
	EXTI->RTSR &= ~(1 << 13);
	EXTI->FTSR |=  (1 << 13);

	/*
	 * Nothing more from the SCC, disable it to prevent accidental
	 * remapping of the interrupt lines.
	 */
	RCC->APB2ENR &= ~(1 << 14);

	////////////////////////////////////////////////////////////////////

	/*
	 *
	 * Per the STM32F411xC/E family datasheet, the handler for EXTI_15_10
	 * is at 40.
	 *
	 * Per the STM32F4 architecture datasheet, the NVIC IP registers
	 * each contain 4 interrupt indices; 0-3 for IPR[0], 4-7 for IPR[1],
	 * and so on. Each index has 8 bits denoting the priority in the top
	 * 4 bits; lower number = higher priority.
	 *
	 * Position 40 in the NVIC table would be at IPR[10][7:0]; or,
	 * alternatively, just IP[40].
	 */
	NVIC->IP[40] = (1 << 4);
	NVIC->IP[6]  = (0b1111 << 4);	// SysTick; make it least-priority

	/*
	 * Per the STM32F4 architecture datasheet, the NVIC ISER/ICER registers
	 * each contain 32 interrupt indices; 0-31 for I{S/C}ER[0], 32-63 for
	 * I{S/C}ER[1], and so on -- one bit per line.
	 *
	 * ISER is written '1' to enable a particular interrupt, while ICER is
	 * written '1' to disable the same interrupt. Writing '0' has no
	 * effect.
	 *
	 * Position 25 in the NVIC table would be at I{S/C}ER[0][25:25]; while
	 * position 27 would be at I{S/C}ER[0][27:27].
	 */
	NVIC->ISER[0] = (1 << 6);	// Note: Writing '0' is a no-op
	NVIC->ISER[1] = (1 << 8);	// Note: Writing '0' is a no-op
	EXTI->IMR |= (1 << 13);		// Unmask the interrupt on Line 13
	TIM2->EGR |= (1 << 0);		// Trigger an update on TIM2
	TIM2->CR1 |= (1 << 0);		// Activate both timers

	/*
	 * Enable tick counting; the idea is to allow main() to perform
	 * periodic tasks.
	 */
	SysTick->LOAD = (20000-1);	// Target is 100 Hz with 2MHz clock
	SysTick->VAL  = 0;
	SysTick->CTRL &= ~(1 << 2);	// Clock base = 16MHz / 8 = 2MHz
	SysTick->CTRL &= ~(1 << 16);
	SysTick->CTRL |= (0b11 << 0);	// Enable the tick

	//Enable ESP8266 reset pin
	GPIOB->MODER &= ~(3 << 2); // Set to input
	GPIOB->MODER |= (1 << 2); // Set to output
	GPIOB->OTYPER |= (1 << 1); // Set to push-pull
	GPIOB->ODR |= (1 << 1); // Set to high


	// Do the initialization of USART last.
	usart2_init();
}

bool getWifiStatus()
{
	struct usart_rx_event usart_evt_6;

	char			rxb_data_6[128];
	unsigned int	rxb_idx_6  = 0;
	unsigned int	rxb_size_6 = 0;

	while(1)
	{
		do
		{
			if (!usart6_rx_get_event(&usart_evt_6))
				// Nothing to do here
				break;
			else if (!usart_evt_6.valid)
				break;

			if (usart_evt_6.is_idle) {
				rxb_size_6 = rxb_idx_6;
				break;
			} else if (!usart_evt_6.has_data) {
				break;
			}

			// Store the data
			if (rxb_idx_6 >= sizeof(rxb_data_6)) {
				rxb_size_6 = rxb_idx_6;
				break;
			}
			rxb_data_6[rxb_idx_6++] = usart_evt_6.c;
			break;
		} while(0);

		if (rxb_size_6 > 0) // If the entered symbol is normal
		{
			usart2_tx_send(rxb_data_6, rxb_size_6);
			for(int i = 0; i < rxb_size_6; i++)
			{
				if(rxb_data_6[i] == 'C')
				{
					rxb_size_6 = rxb_idx_6 = 0;
					return true;

				}

				if(rxb_data_6[i] == 'D')
				{
					rxb_size_6 = rxb_idx_6 = 0;
					return false;
				}
			}

		}
	}
}

int main(void) {
	/*
		Initializes usart6 as well

		usart6 - esp32
		usart2 - pc for debugging
	*/
	do_sys_config();
	ADC_init();

	usart2_tx_send("STM32 starting operations ...", sizeof("STM32 starting operations ..."));

	int adc_value = 0;
	int dequeued_value;

    // Initialize a queue
	Queue* queue = createQueue();

	/* Loop forever */
	while (1) {
		// ADC related functionality
		ADC_enable();
		ADC_startconv();
		ADC_waitconv();
		adc_value = ADC_GetVal();
		bool wifiModuleConnected = getWifiStatus();

		if (wifiModuleConnected && !isEmpty(queue)) {
			// Remove element in queue since it was already sent
			dequeue(queue, &dequeued_value);
		}

		// Message Queue related functionality
		enqueue(queue, adc_value);

		// Send to serial monitor
	    if (!usart6_tx_is_busy() && !isEmpty(queue)) {


	    	int result = peek(queue);
	    	// DequeueResult result = dequeue(queue, &dequeued_value);

			char* jsonDatapoint = createJSONString(result, SENSORTYPE);
			usart6_tx_send(jsonDatapoint, strlen(jsonDatapoint));

			// Send to pc for debugging
			// comment out during deployment
			usart2_tx_send(jsonDatapoint, strlen(jsonDatapoint));

			// Free dynamically allocated memory
			free(jsonDatapoint);
	    }

		delay(SENSORDELAY);
	}
}

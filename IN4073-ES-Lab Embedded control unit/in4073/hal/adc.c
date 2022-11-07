/*------------------------------------------------------------------
 *  adc.c -- 	adc configuration, reads battery voltage after a 1/10 
 *		voltage divider on the power distribution board
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "adc.h"
#include "app_util_platform.h"

uint16_t bat_volt;

//#define BATTERY_VOLTAGE 4 //these are AIN, not ports p0.01 = ain2
//#define BATTERY_AMPERAGE 2

void adc_request_sample(void)
{
	if (!NRF_ADC->BUSY) NRF_ADC->TASKS_START = 1;
}

void ADC_IRQHandler(void)
{
	NRF_ADC->EVENTS_END = 0;
	bat_volt = NRF_ADC -> RESULT*7 - 60; // Battery voltage = (result*1.2*3/255*2) = RESULT*0.007058824
																		// 12.622V is read as 12.320 = diff of 
}

void adc_init(void)
{
	// VREF = BandGap = 1.2V | 2/3 scaling of input | 8bit resolution | AIN2 & AIN4 are connected on board
	NRF_ADC->CONFIG = (ADC_CONFIG_PSEL_AnalogInput4 << ADC_CONFIG_PSEL_Pos) 
							| (ADC_CONFIG_INPSEL_AnalogInputTwoThirdsPrescaling << ADC_CONFIG_INPSEL_Pos);

	/* Enable ADC*/
	NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

	/* Enable interrupt on ADC sample ready event*/     
	NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
	NVIC_SetPriority(ADC_IRQn, 3);
	NVIC_EnableIRQ(ADC_IRQn);
}



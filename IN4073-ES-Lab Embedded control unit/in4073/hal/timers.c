/*------------------------------------------------------------------
 *  timers.c -- TIMER2 is for time-keeping, TIMER1 for motors. 
 *		TIMER0 is for soft-device
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include <stddef.h>
#include "timers.h"
#include "app_util_platform.h"
#include "app_timer.h"
#include "utils/quad_ble.h"
#include "control.h"
#include "nrf_gpio.h"
#include "in4073.h"

// time since start in us
/* static */ uint32_t global_time;
static bool timer_flag;



void TIMER2_IRQHandler(void)
{
	if (NRF_TIMER2->EVENTS_COMPARE[3]) {
		NRF_TIMER2->EVENTS_COMPARE[3] = 0;
		global_time += 312; //2500*0.125 in us

		NRF_TIMER2->TASKS_CAPTURE[2]=1; // This is asking to capture the current counter into CC[2]
		if(!radio_active && (NRF_TIMER2->CC[2] < 500)) {
									// Bound the motor value between 0 and MAX_MOTOR_VAL
			motor[2] = (motor[2] < MAX_MOTOR_VAL) ? ((motor[2] < 0) ? 0: motor[2]) : MAX_MOTOR_VAL;
			motor[3] = (motor[3] < MAX_MOTOR_VAL) ? ((motor[3] < 0) ? 0: motor[3]) : MAX_MOTOR_VAL;
			NRF_TIMER2->CC[0] = 1000 + motor[2];			
			NRF_TIMER2->CC[1] = 1000 + motor[3];	
		}
	}
}

void TIMER1_IRQHandler(void)
{
	if (NRF_TIMER1->EVENTS_COMPARE[3]) {
		NRF_TIMER1->EVENTS_COMPARE[3] = 0;

		NRF_TIMER1->TASKS_CAPTURE[2]=1;
		if(!radio_active && (NRF_TIMER1->CC[2] < 500)) {
			nrf_gpio_pin_set(20);
			motor[0] = (motor[0] < MAX_MOTOR_VAL) ? ((motor[0] < 0) ? 0: motor[0]) : MAX_MOTOR_VAL;
			motor[1] = (motor[1] < MAX_MOTOR_VAL) ? ((motor[1] < 0) ? 0: motor[1]) : MAX_MOTOR_VAL;
			NRF_TIMER1->CC[0] = 1000 + motor[0];			
			NRF_TIMER1->CC[1] = 1000 + motor[1];	
			nrf_gpio_pin_clear(20);
		}
	}
}


uint32_t get_time_us(void)
{
	NRF_TIMER2->TASKS_CAPTURE[2]=1;
	return (global_time+(NRF_TIMER2->CC[2]>>3));
}

bool check_timer_flag(void)
{
	return timer_flag;
}

void clear_timer_flag(void)
{
	timer_flag = false;
}

void quadrupel_timer_handler(void *p_context)
{
	systemCounter++;

	timer_flag = true;
}


void timers_init(void)
{
	global_time = 0;
	timer_flag = false;

	NRF_TIMER2->PRESCALER 	= 0x1UL; // 0.125us //Set prescaler. Higher number gives slower timer.
	NRF_TIMER2->INTENSET    = TIMER_INTENSET_COMPARE3_Msk;
	NRF_TIMER2->CC[0]	= 1000; // motor signal is 125-250us //Set value for TIMER2 compare register 0
	NRF_TIMER2->CC[1]	= 1000; // 
	//	NRF_TIMER2->CC[2]	= 0xffff; // time-keeping
	NRF_TIMER2->CC[3]	= 2500; 
	NRF_TIMER2->SHORTS	= TIMER_SHORTS_COMPARE3_CLEAR_Msk;
	NRF_TIMER2->TASKS_CLEAR = 1;

	NRF_TIMER1->PRESCALER 	= 0x1UL; // 0.125us
	NRF_TIMER1->INTENSET    = TIMER_INTENSET_COMPARE3_Msk;
	NRF_TIMER1->CC[0]	= 1000; // motor signal is 125-250us
	NRF_TIMER1->CC[1]	= 1000; // 
	//	NRF_TIMER1->CC[2]	= 1000; // is used to know "when" we are in the motor pulse & timekeeping
	NRF_TIMER1->CC[3]	= 2500;
	NRF_TIMER1->SHORTS	= TIMER_SHORTS_COMPARE3_CLEAR_Msk;
	NRF_TIMER1->TASKS_CLEAR = 1;

	NRF_TIMER2->TASKS_START	= 1;
	NRF_TIMER1->TASKS_START	= 1;

	NVIC_ClearPendingIRQ(TIMER2_IRQn);
	NVIC_SetPriority(TIMER2_IRQn, 1);
	NVIC_EnableIRQ(TIMER2_IRQn);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_SetPriority(TIMER1_IRQn, 1);
	NVIC_EnableIRQ(TIMER1_IRQn);

	// motor 0 - gpiote 0
	NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
	NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
	NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[3];
	NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];

	// motor 1 - gpiote 1
	NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[1];
	NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];
	NRF_PPI->CH[3].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[3];
	NRF_PPI->CH[3].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

	// motor 2 - gpiote 2
	NRF_PPI->CH[4].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
	NRF_PPI->CH[4].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[2];
	NRF_PPI->CH[5].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[3];
	NRF_PPI->CH[5].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[2];

	// motor 3 - gpiote 3
	NRF_PPI->CH[6].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[1];
	NRF_PPI->CH[6].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[3];
	NRF_PPI->CH[7].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[3];
	NRF_PPI->CH[7].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[3];

	NRF_PPI->CHENSET = PPI_CHENSET_CH0_Msk | PPI_CHENSET_CH1_Msk | PPI_CHENSET_CH2_Msk | PPI_CHENSET_CH3_Msk | PPI_CHENSET_CH4_Msk | PPI_CHENSET_CH5_Msk | PPI_CHENSET_CH6_Msk | PPI_CHENSET_CH7_Msk;


	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

	APP_TIMER_DEF(quadrupel_timer);
	app_timer_create(&quadrupel_timer, APP_TIMER_MODE_REPEATED, quadrupel_timer_handler);
	app_timer_start(quadrupel_timer, QUADRUPEL_TIMER_PERIOD, NULL);
}

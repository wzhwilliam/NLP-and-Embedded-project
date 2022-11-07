#ifndef TIMERS_H_
#define TIMERS_H_

#include <stdbool.h>
#include <inttypes.h>

// app timer
#define TIMER_PERIOD				50 //50ms=20Hz (MAX 23bit, 4.6hours)
#define APP_TIMER_PRESCALER			0
#define APP_TIMER_OP_QUEUE_SIZE		4
#define QUADRUPEL_TIMER_PERIOD		APP_TIMER_TICKS(TIMER_PERIOD, APP_TIMER_PRESCALER) // timer period in ticks


// time since start in us
extern uint32_t global_time;

void timers_init(void);
uint32_t get_time_us(void);
bool check_timer_flag(void);
void clear_timer_flag(void);


#endif /* TIMERS_H_ */

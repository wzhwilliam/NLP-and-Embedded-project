#ifndef UART_H_
#define UART_H_

#include "../utils/queue.h"
#include <stdbool.h>

extern Queue rx_queue;
extern Queue tx_queue;

void uart_init(void);
void uart_put(uint8_t byte, bool blocking);

#endif /* HAL_UART_H_ */

/*------------------------------------------------------------------
 *  uart.c -- configures uart
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */
#include "uart.h"
#include <stdbool.h>
#include <stdio.h>
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "utils/quad_ble.h"
#include "in4073.h"
#include "utils/queue.h"
#include "control.h"

// My include
#include "nrf_delay.h"

#define RX_PIN_NUMBER  16
#define TX_PIN_NUMBER  14

Queue rx_queue;
Queue tx_queue;

static bool txd_available = true;


/**
 * @brief Function to send one character to PC.
 * Uses bluetooth in wireless mode, else uses uart.
 * When called with blocking = true param, the function is waiting until
 * there's a free spot in the rx queue (this is used when sending log).
 * @param uint8_t One byte to send
 * @param bool Blocking mode - set to true if you want to use the uart in blocking mode 
 * (in blocking mode it waits until every byte has been sent successfully)
 * @author Kristóf
 */
void uart_put(uint8_t byte, bool blocking)
{
	// Don't send anything in wireless mode
	if (!wireless_mode)
	{
		NVIC_DisableIRQ(UART0_IRQn);

		if (txd_available) // Send immediately if possible
		{
			txd_available = false;
			NRF_UART0->TXD = byte;
		} 
		else // Put character in queue
		{
			// Wait for free spot in queue in blocking mode
			if (blocking)
			{
				while (!enqueue(&tx_queue, byte))	// Wait until there's a spot in queue
				{
					NVIC_EnableIRQ(UART0_IRQn);		// Enable IRQ
					nrf_delay_us(50);				// Wait 50 us to empty the queue
					NVIC_DisableIRQ(UART0_IRQn);	// Disable IRQ again
				}
			}
			// Don't care if the placement in the queue wasn't successful in non-blocking mode
			else
			{
				enqueue(&tx_queue, byte);
			}
		}

		NVIC_EnableIRQ(UART0_IRQn);
	}
}

/**
 * @brief Reroute printf
 * @param int Not used, but was a part of the example code
 * @param char* String to send on uart
 * @param int Length of the string
 * @return The length of the string which was sent
 * @author Modified by Kristóf
 */
int _write(int file, const char * p_char, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		uart_put(*p_char++, false);
	}

	return len;
}


void UART0_IRQHandler(void)
{
	if (NRF_UART0->EVENTS_RXDRDY != 0) {
		NRF_UART0->EVENTS_RXDRDY  = 0;
		enqueue(&rx_queue, NRF_UART0->RXD);
	}

	if (NRF_UART0->EVENTS_TXDRDY != 0) {
		NRF_UART0->EVENTS_TXDRDY = 0;
		if (tx_queue.count) NRF_UART0->TXD = dequeue(&tx_queue);
		else txd_available = true;
	}

	if (NRF_UART0->EVENTS_ERROR != 0) {
		NRF_UART0->EVENTS_ERROR = 0;
		printf("uart error: %lu\n", NRF_UART0->ERRORSRC);
	}
}

void uart_init(void)
{
	init_queue(&rx_queue); // Initialize receive queue
	init_queue(&tx_queue); // Initialize transmit queue

	nrf_gpio_cfg_output(TX_PIN_NUMBER);
	nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_NOPULL); 
	NRF_UART0->PSELTXD = TX_PIN_NUMBER;
	NRF_UART0->PSELRXD = RX_PIN_NUMBER;
	NRF_UART0->BAUDRATE        = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);

	NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->EVENTS_RXDRDY    = 0;
	NRF_UART0->EVENTS_TXDRDY    = 0;
	NRF_UART0->TASKS_STARTTX    = 1;
	NRF_UART0->TASKS_STARTRX    = 1;

	NRF_UART0->INTENCLR = 0xffffffffUL;
	NRF_UART0->INTENSET = 	(UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos) |
			(UART_INTENSET_TXDRDY_Set << UART_INTENSET_TXDRDY_Pos) |
			(UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

	NVIC_ClearPendingIRQ(UART0_IRQn);
	NVIC_SetPriority(UART0_IRQn, 3);
	NVIC_EnableIRQ(UART0_IRQn);
}

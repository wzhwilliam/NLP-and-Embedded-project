#ifndef BLE_H_
#define BLE_H_

#include "queue.h"

extern Queue ble_rx_queue;
extern Queue ble_tx_queue;

extern volatile bool radio_active;

void quad_ble_init(void);
uint32_t quad_ble_send(void);


#endif /* BLE_H_ */

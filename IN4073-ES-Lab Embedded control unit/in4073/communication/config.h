/*
*   This is a file for defining some configurable data
*   This is not the best solution, but way faster than 
*   write some serialization stuff in C/C++ for json/xml
*   files. 
*/

#ifndef CONFIG_H_
#define CONFIG_H_

// PC side defines
#define SEND_RATE_MS 50
#define DEBUG_MODE 1
#define SERIAL_PORT "/dev/ttyUSB0"
#define BT_PORT "/dev/pts/3"

// Drone config data defines
#define P_VALUE 12
#define P1_VALUE 5
#define P2_VALUE 60
#define HEIGHT_GAIN 5

#endif /* CONFIG_H_ */
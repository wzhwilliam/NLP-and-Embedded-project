#ifndef PROTOCOL_H__
#define PROTOCOL_H__

// Buffer and array size defines
#define BUF_SIZE 50
#define CFG_SIZE 8
#define CMD_SIZE 7

#define TEXT_LEN 1024*128

#define UART 0
#define BLUETOOTH 1

#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "joy.h"
#include "config.h"

// Message types enum
typedef enum 
{ 
	EMG,	// Emergency stop -> Is it necessary???
    CFG, 	// Send config params
    MODE, 	// Change mode
    CMD,	// Position commands
	TELEM,	// Telemetry data
	LOG,	// Logged data
	ACK,	// ACK message
	DEBUG	// Debug message
} msgType;

// System states enum
typedef enum 
{
	SafeMode = 0, 			// Init, motors off, waiting for mode change
	PanicMode = 1, 			// Ramping down motors for 3 sec
	ManualMode = 2, 		// Simple motor control
	CalibrationMode = 3,	// Calibrate sensor
	YawControlledMode = 4,	// Simple P controller for yaw
	FullControllMode = 5,	// Control all 3 axes
	RawMode = 6,			// Use raw sensor data
	HeightControl = 7,		// Height control with barometer
	WirelessControl = 8,	// Use bluetooth for communication
} SystemState_t;

// Receiver state machine states enum
typedef enum
{
	START, TYPE, LENGTH, DATA, CSUM, SDEBUG
} recState;

// Struct for receiver state machine variables
typedef struct
{
	recState actualState;
	uint8_t recCsum;
	uint8_t recLength;
	msgType recType;
	uint8_t msg[BUF_SIZE];
	uint8_t idx;
} recMachine;

// Logging types enum
typedef enum {
	Telemetry,
	ModeChg,
	Command,
	Profiling,
	Full
} logType;

// Struct to pass the pointers to the msg process function
typedef struct
{
    char* text;
    float* motorValues;
    uint8_t* ackMode;
    int16_t* gains;
} pointers;

#ifdef __cplusplus
extern "C" {
#endif

// Functions to split multi-byte variables to single bytes
void ui16_to_ui8(uint16_t source, uint8_t *dest);
void i16_to_ui8(int16_t source, uint8_t *dest);
void ui32_to_ui8(uint32_t source, uint8_t *dest);
void i32_to_ui8(int32_t source, uint8_t *dest);

// Functions to convert back multi-byte values
uint16_t to_ui16(uint8_t *pData);
int16_t to_i16(uint8_t *pData);
uint32_t to_ui32(uint8_t *pData);
int32_t to_i32(uint8_t *pData);

// Functions used at PC <-> drone communication
int processMsg(msgType type, uint8_t *pData);
int processMsgGui(msgType type, uint8_t *pData, pointers pointers);
void packMessage(msgType type, uint8_t *pData);
int unpackMessage(uint8_t c, recMachine *SM);
int unpackMessageGui(uint8_t c, recMachine *SM, pointers pointers);
int8_t processKeyboard(char c, uint8_t *cmd);

// TCP socket for processing
void openSocket();
void closeSocket();

// Console I/O
void term_initio();
void term_exitio();
void term_puts(char *s);
void term_putchar(char c);
int	term_getchar_nb();
int	term_getchar();

// Uart + bluetooth serial I/O
void serial_port_open(const char *serial_device);
void ble_port_open(const char *serial_device);
void serial_port_close(void);
void ble_port_close(void);
int serial_port_getchar(bool ble);
int serial_port_putchar(char c);
int get_fd_ble();
int get_fd_serial();

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_H__
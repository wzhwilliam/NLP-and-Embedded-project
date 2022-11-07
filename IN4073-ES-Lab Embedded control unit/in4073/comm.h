#ifndef COMM_H_
#define COMM_H_

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include "in4073.h"
#include "control.h"
#include "utils/quad_ble.h"

// Buffer and array size defines
#define BUF_SIZE 20
#define CMD_SIZE 7
#define TELEM_SIZE 39
#define PROFILING_SIZE 32
#define LOG_SIZE TELEM_SIZE+5

// Bool array containing the key presses
extern bool keys[18];
// Key value location in the keys variable
#define A_KEY 0 	// Increase throttle
#define Z_KEY 1 	// Decrease throttle
#define LEFT_KEY 2 	// Trim roll left
#define RIGHT_KEY 3 // Trim roll right
#define UP_KEY 4 	// Trim pitch forward
#define DOWN_KEY 5 	// Trim pitch backwards
#define Q_KEY 6 	// Trim yaw left
#define W_KEY 7 	// Trim yaw right
#define ESC_KEY 8 	// Abort
#define JOY_FIRE 9 	// Abort
#define U_KEY 10 	// Yaw control P up
#define J_KEY 11 	// Yaw control P down
#define I_KEY 12 	// Roll/pitch control P1 up
#define K_KEY 13 	// Roll/pitch control P1 down
#define O_KEY 14 	// Roll/pitch control P2 up
#define L_KEY 15 	// Roll/pitch control P2 down
#define Y_KEY 16 	// Height control gain up
#define H_KEY 17 	// Height control gain down

extern uint8_t joystickRoll;
extern uint8_t joystickPitch;
extern uint8_t joystickYaw;
extern uint8_t joystickThrottle;

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

// Receiver state machine states enum
typedef enum
{
	START, TYPE, LENGTH, DATA, CSUM
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

void comm_init();

extern uint32_t flashAdr;
extern uint32_t storedRows;

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

// Functions used at communication
void processMsg(msgType type, uint8_t *pData);
void packMessage(msgType type, uint8_t *pData, const char* debugMsg);
void unpackMessage(uint8_t c, recMachine *SM);

bool checkConnection();

// Log types enum
typedef enum {
	Telemetry,
	ModeChg,
	Command,
	l_Profiling,
	Full
} logType;

// Telemetry functions
void serializeTelemetry(telemetry *telem, uint8_t *pData);
void sendTelemetry(uint8_t *telemData);

// Profiling functions
void serializeProfiling(profilingTelemetry *profilingTelem, uint8_t *pData);
void sendProfilingData(profilingTelemetry *profilingTelem);

// Functions for logging
void saveLog(logType type, uint8_t *pData);
void sendLog();

#endif /* COMM_H_ */
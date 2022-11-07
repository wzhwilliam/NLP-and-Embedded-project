/*------------------------------------------------------------------
 *  in4073.h -- defines, globals, function prototypes
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */
#ifndef IN4073_H__
#define IN4073_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "utils/profiling.h"

enum SystemState_t {
	SafeMode = 0, 			// Init, motors off, waiting for mode change
	PanicMode = 1, 			// Ramping down motors for 3 sec
	ManualMode = 2, 		// Simple motor control
	CalibrationMode = 3,	// Calibrate sensor
	YawControlledMode = 4,	// Simple P controller for yaw
	FullControllMode = 5,	// Control all 3 axes
	RawMode = 6,			// Use raw sensor data
	HeightControl = 7,		// Height control with barometer
	WirelessControl = 8	    // Use bluetooth for communication
};
extern enum SystemState_t systemState;
extern uint32_t systemCounter;
extern profilingData profileData; 
int16_t phi_trim, psi_trim, theta_trim;
int32_t _phi_trim, _psi_trim, _theta_trim;
int16_t sr_trim, sp_trim, sq_trim;
int32_t _sr_trim, _sp_trim, _sq_trim;

bool setSystemState(enum SystemState_t newState);
void finishFlying();

// Struct to store telemetry data
typedef struct {
	uint8_t mode;
	int16_t motor1;
	int16_t motor2;
	int16_t motor3;
	int16_t motor4;
	int16_t phi;
	int16_t theta;
	int16_t psi;
	int16_t sp;
	int16_t sq;
	int16_t sr;
	uint16_t bat;
	int32_t temp;
	int32_t pres;
	int16_t p;
	int16_t p1;
	int16_t p2;
	int16_t hei;
} telemetry;

typedef struct {
	uint32_t controlLoopTime;
	uint32_t timerFlagTime;
	uint32_t yawModeTime;
	uint32_t fullModeTime;
	uint32_t rawModeTime;
	uint32_t heightModeTime;
	uint32_t loggingTime;
	uint32_t sqrtTime;

} profilingTelemetry;

#endif // IN4073_H__

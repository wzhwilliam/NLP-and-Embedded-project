#ifndef CONTROL_H_
#define CONTROL_H_

#include <inttypes.h>
#include <stdbool.h>

#define MAX_MOTOR_VAL 1000
#define CALIBRATION_SAMPLES 200

typedef struct {
    int8_t calibrationCycle;
} calibrationData;

extern uint16_t motor[4];
extern int16_t ae[4];
extern bool wireless_mode;
extern calibrationData cData;

extern int16_t Gain_Yaw;
extern int16_t Gain_P1;
extern int16_t Gain_P2;
extern int16_t Gain_height;

// extern int16_t light;
bool checkJoystickThrottle();

void initializeMotorControl();
void run_filters_and_control();

uint16_t calculateHeight(int32_t temp, int32_t pressure);

unsigned int isqrt( unsigned int y );

int32_t pressure_pre;
int16_t throttle_pre;



#endif /* CONTROL_H_ */

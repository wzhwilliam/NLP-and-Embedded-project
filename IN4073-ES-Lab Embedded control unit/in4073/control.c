/*------------------------------------------------------------------
 *  control.c -- here you can implement your control algorithm
 *		 and any motor clipping or whatever else
 *		 remember! motor input =  0-1000 : 125-250 us (OneShot125)
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "control.h"
#include "math.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "in4073.h"
#include "hal/barometer.h"
#include "utils/quad_ble.h"
#include "utils/queue.h"
#include "mpu6050/mpu6050.h"
#include "uart.h"
#include "gpio.h"
#include "comm.h"
#include "utils/profiling.h" 
#include "filter.h"

#include <math.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) > (y)) ? (y) : (x))

#define KEYDEBOUNCE 20 // 50ms periods * 20 = 1s
#define MOTOR_TURN_MINIMUM 220
#define THROTTLE_START_THRESHOLD 10

#define B_CONSTANT 6000
#define D_CONSTANT 4000

#define JOYSTICK_THROTTLE_MAX 600

#define JOYSTICK_PR_DIVIDER 20
#define JOYSTICK_YAW_MUTIPLIER 15
#define JOYSTICK_YAW_DIVIDER_MANUAL 2
#define JOYSTICK_THROTTLE_DIVIDER 1
#define JOYSTICK_YAW_MUTIPLIER 15 // YawControllMode MUTIPLiER
#define JOYSTICK_ROLL_MUTIPLIER 30 // RollControllMode MUTIPLiER
#define JOYSTICK_PITCH_MUTIPLIER 30 // PitchControllMode MUTIPLiER

uint16_t motor[4];
int16_t ae[4] = {0, 0, 0, 0};
int16_t te[4] = {0, 0, 0, 0};
bool wireless_mode = false;
// int16_t light;
int16_t sr_pre;//sr we want according to joystickYaw
int16_t phi_pre;//phi we want according to joystickRoll
int16_t theta_pre;//theta we want according to joystickPitch
// int16_t saz_pre;
int16_t sq_filtered, sp_filtered;
int16_t sa_phi, sa_theta, sa_phi_filtered, sa_theta_filtered;
KalmanState_t kS_theta, kS_phi;
ButterWorthState_t bS_sq, bS_sp, bS_sa_theta, bS_sa_phi, bS_sa_sr, bS_pressure;

int16_t Gain_Yaw = 12;//18;//The Gain value of P controller in YawControlMode

int16_t Gain_P1 = 5;//The Gain value of P1 controller in FullControlMode
int16_t Gain_P2 = 60;//The Gain value of P2 controller in FullControlMode
 int16_t Gain_height=100;//The Gain value of P controller in HeightControlMode
// int16_t Gain_height1=5;//The Gain value of P controller in HeightControlMode
// int16_t Gain_height2=5;
// int16_t height_threshold=100;
calibrationData cData;

// When was the last key command processed
static uint32_t lastKeyProcess = 0;



char msg[100];

/**
 * @brief Set motor values to off. This is the most direct way to do so, motor vars are directly read by timer interrupts
 * @author Philip Groet
 */
void motors_off() {
	ae[0] = ae[1] = ae[2] = ae[3] = 0;
	motor[0] = motor[1] = motor[2] = motor[3] = 0;
}

/**
 * @brief Updates the motor values based on the current values in the ae array.
 * Makes sure that the motor values stays between the min an max motor values.
 * Also applies trimming.
 * @author Philip Groet
 */
void update_motors(void)
{
	// If one of ae is unrealistically high, go to panic
	if (
		   ae[0] > 2000
		|| ae[1] > 2000
		|| ae[2] > 2000
		|| ae[3] > 2000
	) {
		snprintf(msg, 100, "One of motor values too high: %d %d %d %d", ae[0], ae[1], ae[2], ae[3]);
		packMessage(DEBUG, NULL, msg);

		if(setSystemState(PanicMode))
		{
			// Send ACK
			uint8_t  ack = PanicMode;
			packMessage(ACK, &ack, NULL);
		}
	}

	// Dont do trim and motor when joystick zero like
	if (!checkJoystickThrottle()) {
		return;
	}

	motor[0] = MIN(MAX(0, ae[0] + te[0]), MAX_MOTOR_VAL);
	motor[1] = MIN(MAX(0, ae[1] + te[1]), MAX_MOTOR_VAL);
	motor[2] = MIN(MAX(0, ae[2] + te[2]), MAX_MOTOR_VAL);
	motor[3] = MIN(MAX(0, ae[3] + te[3]), MAX_MOTOR_VAL);
}

/**
 * @brief Apply throttle trimming
 * 
 * @author Wesley de Hek
 * @param n - amount of trimming.
 */
void increaseThrottle(int n) {
	te[0] += n;
	te[1] += n;
	te[2] += n;
	te[3] += n;
}

/**
 * @brief Apply roll trimming
 * 
 * @author Wesley de Hek
 * @param degrees - amount of trimming.
 */
void increaseRoll(int degrees) {
	te[1] -= degrees;
	te[3] += degrees;
}

/**
 * @brief Apply pitch trimming
 * 
 * @author Wesley de Hek
 * @param degrees - amount of trimming.
 */
void increasePitch(int degrees) {
	//If degrees are negative lower speed of ae[2]
	te[0] -= degrees;
	te[2] += degrees;
	//Get current motor value from motor array;
}

/**
 * @brief Apply Yaw trimming
 * 
 * @author Wesley de Hek
 * @param degrees - amount of trimming.
 */
void increaseYaw(int degrees) {
	te[0] -= degrees;
	te[1] += degrees;
	te[2] -= degrees;
	te[3] += degrees;
}

/**
 * @brief Calculates the basic motor values, based on yaw/roll/pitch/throttle and sets them in ae.
 * 
 * @author Philip Groet & Wesley de Hek
 * @param Z - Desired lift.
 * @param M - Desired pitch.
 * @param N - Desired yaw.
 * @param L - Desired roll.
 */
void calculateMotorValues(int16_t Z, int16_t M, int16_t N, int16_t L) {
	ae[0] = isqrt((MAX(0, ((Z+2*M) * B_CONSTANT - N * D_CONSTANT)) / 4));
	ae[1] = isqrt((MAX(0, ((Z-2*L) * B_CONSTANT + N * D_CONSTANT)) / 4));
	ae[2] = isqrt((MAX(0, ((Z-2*M) * B_CONSTANT - N * D_CONSTANT)) / 4));
	ae[3] = isqrt((MAX(0, ((Z+2*L) * B_CONSTANT + N * D_CONSTANT)) / 4));

	// Minimum value to keep rotors spinning
	ae[0] = MAX(MOTOR_TURN_MINIMUM, ae[0]);
	ae[1] = MAX(MOTOR_TURN_MINIMUM, ae[1]);
	ae[2] = MAX(MOTOR_TURN_MINIMUM, ae[2]);
	ae[3] = MAX(MOTOR_TURN_MINIMUM, ae[3]);
}

/**
 * @brief Processes the basic key commands that are used in all flying modes. 
 * @author Philip Groet & Wesley de Hek & Zhiheng Wang
 */
void processBasicKeyPress(enum SystemState_t sysState) {
	//Don't process key commands in safe or panic mode:
	if(sysState == PanicMode || sysState == SafeMode) {
		return;
	}

	if (keys[A_KEY]) {
		increaseThrottle(10);
		lastKeyProcess = systemCounter;
	}
	if (keys[Z_KEY]) {
		increaseThrottle(-10);
		lastKeyProcess = systemCounter;
	}
	if (keys[UP_KEY]) { //pitch forward
		increasePitch(10); 
		lastKeyProcess = systemCounter;
	}
	if(keys[DOWN_KEY]) { // pitch backwards
		increasePitch(-10);
		lastKeyProcess = systemCounter;
	}
	if(keys[LEFT_KEY]) {
		increaseRoll(-10);
		lastKeyProcess = systemCounter;
	}
	if(keys[RIGHT_KEY]) {
		increaseRoll(10);
		lastKeyProcess = systemCounter;
	}
	if(keys[Q_KEY]) {
		increaseYaw(10);
		lastKeyProcess = systemCounter;
	}
	if(keys[W_KEY]) {
		increaseYaw(-10);
		lastKeyProcess = systemCounter;
	}

	//Yaw gain only possible in YAW, FULL, and RAW mode: 
	if(sysState == YawControlledMode || sysState == FullControllMode || sysState == RawMode) {
		if (keys[U_KEY]) { //Increase Yaw gain
			if (Gain_Yaw >= 2)
			{
				Gain_Yaw -= 1;
			}
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_Yaw: %d", Gain_Yaw);
			packMessage(DEBUG, NULL, msg);
		}

		if (keys[J_KEY]) { //Decrease Yaw gain
			Gain_Yaw += 1;
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_Yaw: %d", Gain_Yaw);
			packMessage(DEBUG, NULL, msg);

		}
	}

	//P1 & P2 gain only possible in FULL and RAW mode:
	if(sysState == FullControllMode || sysState == RawMode) {
		if (keys[I_KEY]) {
			Gain_P1 += 1;
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_P1: %d", Gain_P1);
			packMessage(DEBUG, NULL, msg);
		}

		if (keys[K_KEY]) {
			if (Gain_P1 >= 2) {
				Gain_P1 -= 1;
			}
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_P1: %d", Gain_P1);
			packMessage(DEBUG, NULL, msg);

		}

		if (keys[O_KEY]) {
			if (Gain_P2 >= 2)
			{
				Gain_P2 -= 1;
			}
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_P2: %d", Gain_P2);
			packMessage(DEBUG, NULL, msg);
		}

		if (keys[L_KEY]) {
			Gain_P2 += 1;
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_P2: %d", Gain_P2);
			packMessage(DEBUG, NULL, msg);

		}
	}

	//Height gain only possible in height mode:
	if(sysState == HeightControl) {
		if (keys[Y_KEY]) { //Increase height gain
			Gain_height += 1;
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_Height: %d", Gain_height);
			packMessage(DEBUG, NULL, msg);
		}

		if (keys[H_KEY]) { //Decrease height gain
			if (Gain_height >= 2)
			{
				Gain_height -= 1;
			}
			
			lastKeyProcess = systemCounter;

			snprintf(msg, 100, "Gain_Height: %d", Gain_height);
			packMessage(DEBUG, NULL, msg);

		}
	}
}

/**
 * @brief Checks if the joystick throttle is above the threshold.
 * 
 * @author Wesley de Hek
 * @return true - Joystick throttle is above the threshold.
 * @return false - Joystick throttle is below the threshold.
 */
bool checkJoystickThrottle() {
	if(joystickThrottle <= THROTTLE_START_THRESHOLD) {
		motors_off(); //Turning off motors:

		return false;
	}

	return true;
}

/**
 * @brief Called when drone is in panic mode, first levels the drone (if needed) and than slowly lets it land on the ground.
 * 
 * @author Wesley de Hek
 */
void processPanicMode() {
	//Level drone, looking at current motor values and taking the average:
	if(motor[0]-te[0] != motor[1]-te[1] || motor[0]-te[0] != motor[2]-te[2] || motor[0]-te[0] != motor[3]-te[3]) {
		uint16_t averageMotorValue = (motor[0] + motor[1] + motor[2] + motor[3])/4;

		ae[0] = ae[1] = ae[2] = ae[3] = averageMotorValue;

		return;
	}

	//Turning motors off when below minimum turning speed:
	if(motor[0] <= MOTOR_TURN_MINIMUM && motor[1] <= MOTOR_TURN_MINIMUM && motor[2] <= MOTOR_TURN_MINIMUM && motor[3] <= MOTOR_TURN_MINIMUM) {
		motors_off();
	}

	//Decreasing motor speed by 0.05% every iteration, making it a soft landing:
	for(int i = 0; i < 4 ; i++) {
		if(motor[i] >= MOTOR_TURN_MINIMUM) {
			ae[i] = motor[i] - (0.0005*motor[i]);
		}
	}
}

/**
 * @brief Calculates the basic throttle value from the joystick.
 * 
 * @author Wesley de Hek, 
 * @return int16_t - throttle value.
 */
int16_t calculateBasicThrottle() {
	int16_t j =  ((int16_t) joystickThrottle - THROTTLE_START_THRESHOLD) / JOYSTICK_THROTTLE_DIVIDER;
	return MIN(j, JOYSTICK_THROTTLE_MAX);
}

/**
 * @brief Process manual mode motor control.
 * 
 * @author Philip Groet
 */
void processManualMode() {
	if(!checkJoystickThrottle()) {
		return;
	}

	int16_t Z, M, N, L;

	// throttle
	Z = calculateBasicThrottle();
	// pitch
	M = -1 * ((int16_t)joystickPitch - 127) / JOYSTICK_PR_DIVIDER;
	// roll
	L =      ((int16_t)joystickRoll  - 127) / JOYSTICK_PR_DIVIDER;
	// Yaw
	N =      ((int16_t)joystickYaw - 127)   / JOYSTICK_YAW_DIVIDER_MANUAL;

	calculateMotorValues(Z, M, N, L);	
}

/**
 * @brief Process Yaw mode motor control.
 * 
 * @author Zhiheng Wang
 */
void processYawControl() {
	if(!checkJoystickThrottle()) {
		return;
	}

	startProfiling(p_YawMode);

	int16_t Z, M, N, L;

	// throttle
	Z = calculateBasicThrottle();
	// pitch
	M = -1 * ((int16_t)joystickPitch - 127) / JOYSTICK_PR_DIVIDER;
	// roll
	L =      ((int16_t)joystickRoll  - 127) / JOYSTICK_PR_DIVIDER;
	// Yaw
	sr_pre = -((int16_t)joystickYaw - 127) * JOYSTICK_YAW_MUTIPLIER;//define the yaw rate we want
	N =      -(int16_t)((sr_pre - (sr - sr_trim)) / Gain_Yaw);//Achieve P control in YawControlMode

	calculateMotorValues(Z, M, N, L);
	
	stopProfiling(p_YawMode, true, &profileData);
}

/**
 * @brief Process FULL mode motor control.
 * 
 * @author Zhiheng Wang
 */
void processFullControl() {
	if(!checkJoystickThrottle()) {
		return;
	}

	startProfiling(p_FullControl);

	int16_t Z, M, N, L, sroll, spitch;

	// throttle
	Z = calculateBasicThrottle();

	// pitch
	theta_pre = ((int16_t)joystickPitch  - 127)*32767/127 / JOYSTICK_PR_DIVIDER ;//define the pitch position we want
	spitch = (int16_t)(((theta_pre-(theta - theta_trim))*Gain_P1)*3600/32767);//transfer to the pitch rate we want
	M = (int16_t)((-spitch+(sq - sq_trim))/Gain_P2);//Achieve P control on the pitch direction in FullControllMode

	// roll
	phi_pre = (((int16_t)joystickRoll  - 127))*32767/127 / JOYSTICK_PR_DIVIDER; //define the roll position we want
	sroll =  (int16_t)(((phi_pre - (phi - phi_trim))    *Gain_P1)*3600/32767); // transfer to the roll rate we want
	L = (int16_t)((sroll-(sp-sp_trim))/Gain_P2); //Achieve P control on the roll direction in FullControllMode

	// Yaw
	sr_pre = -((int16_t)joystickYaw - 127) * JOYSTICK_YAW_MUTIPLIER;//define the yaw rate we want
	N =      -(int16_t)((sr_pre - (sr-sr_trim)) / Gain_Yaw);//Achieve P control on the Yaw direction in FullControllMode

	calculateMotorValues(Z, M, N, L);

	stopProfiling(p_FullControl, true, &profileData);
}

/**
 * @brief Process Height mode motor control.
 * 
 * @author Wesley de Hek, Philip Groet ,Zhiheng Wang
 */
void processHeightControl() {
	if(!checkJoystickThrottle()) {
		return;
	}

	//If throttle changes by 20, revert back to full controll mode:
	if(joystickThrottle-throttle_pre > 20 || throttle_pre-joystickThrottle > 20) {
		if(setSystemState(FullControllMode))
		{
			// Send ACK
			uint8_t ack = FullControllMode;
			packMessage(ACK, &ack, NULL);
		}

		return;
	}

	startProfiling(p_HeightMode);

	int16_t Z, M, N, L, sroll, spitch;
	// throttle
	// pressure=butterworth(&bS_pressure, pressure, BW_COEFf_Height_Y_i, BW_COEFf_Height_Y_i1);
	// saz_pre=(int16_t)((pressure_pre-pressure)*Gain_height1);
	// Z =  calculateBasicThrottle() - (int16_t)((saz_pre-saz)*Gain_height2);
	// if(pressure_pre-pressure>height_threshold)
	// {
	// 	light=0;
	// }
	// else if(pressure-pressure_pre>height_threshold)
	// {
	// 	light=1;
	// }
	// else
	// {
	// 	light=2;
	// }
	Z =  calculateBasicThrottle() - (int16_t)((pressure_pre-butterworth(&bS_pressure, pressure, BW_COEFf_Height_Y_i, BW_COEFf_Height_Y_i1))*Gain_height);
	//same with FullControlMode in other directions
	
	// pitch
	theta_pre = ((int16_t)joystickPitch  - 127)*32767/127 / 5;
	spitch = (int16_t)(((theta_pre-(theta - theta_trim))*Gain_P1)*3600/32767);
	M = (int16_t)((-spitch+(sq - sq_trim))/Gain_P2);

	// roll
	phi_pre = (((int16_t)joystickRoll  - 127))*32767/127 / 5; // rate p control
	sroll =  (int16_t)(((phi_pre - (phi - phi_trim))    *Gain_P1)*3600/32767); // abs pos p control
	L = (int16_t)((sroll-(sp-sp_trim))/Gain_P2); // Roll component

	// Yaw
	sr_pre = -((int16_t)joystickYaw - 127) * JOYSTICK_YAW_MUTIPLIER;//define the sr we want
	N =      -(int16_t)((sr_pre - (sr-sr_trim)) / Gain_Yaw);

	calculateMotorValues(Z, M, N, L);
			
	stopProfiling(p_HeightMode, true, &profileData);
}

/**
 * @brief Process Raw mode motor control.
 * 
 * @author Philip Groet
 */
void processRawMode() {
	if(!checkJoystickThrottle()) {
		return;
	}
	
	startProfiling(p_RawMode);

	int16_t Z, M, N, L, sroll, spitch;

	// throttle
	Z = calculateBasicThrottle();

	// // pitch
	sa_theta = sax; // say = sin(theta)*g = theta * g = theta*10
	sa_theta_filtered = butterworth(&bS_sa_theta, sa_theta, BW_COEFf_Raw_Y_i, BW_COEFf_Raw_Y_i1);
	sq_filtered = butterworth(&bS_sq, sq, BW_COEFf_Raw_Y_i, BW_COEFf_Raw_Y_i1);
	kalman(&sq_filtered, &theta, &kS_theta, -sa_theta_filtered, -sq_filtered);

	theta_pre = ((int16_t)joystickPitch  - 127)*32767/127 / 5;
	spitch = (int16_t)(((theta_pre-(theta - theta_trim))*Gain_P1)*3600/32767);
	M = (int16_t)((-spitch+(sq_filtered - sq_trim))/Gain_P2);

	// // roll
	sa_phi = say; // say = sin(theta)*g = theta * g = theta*10
	sa_phi_filtered = butterworth(&bS_sa_phi, sa_phi, BW_COEFf_Raw_Y_i, BW_COEFf_Raw_Y_i1);
	sp_filtered = butterworth(&bS_sp, sp, BW_COEFf_Raw_Y_i, BW_COEFf_Raw_Y_i1);
	kalman(&sp_filtered, &phi, &kS_phi, sa_phi_filtered, sp_filtered);

	phi_pre = (((int16_t)joystickRoll  - 127))*32767/127 / 5; // rate p control
	sroll =  (int16_t)(((phi_pre - (phi - phi_trim))    *Gain_P1)*3600/32767); // abs pos p control
	L = (int16_t)((sroll-(sp_filtered-sp_trim))/Gain_P2); // Roll component

	//Yaw, assignment manual tells us to use butterworth here?
	sr_pre = -((int16_t)joystickYaw - 127) * JOYSTICK_YAW_MUTIPLIER;//define the sr we want

	sr = butterworth(&bS_sa_sr, sr, BW_COEFf_Raw_Y_i, BW_COEFf_Raw_Y_i1);

	N =      -(int16_t)((sr_pre - (sr-sr_trim)) / Gain_Yaw);

	calculateMotorValues(Z, M, N, L);

	stopProfiling(p_RawMode, true, &profileData);
}


/**
 * @brief Initializes the motor control, by zeroing out the motor values at start.
 * 
 * @author Wesley de Hek
 */
void initializeMotorControl() {
	motors_off();
}

/**
 * @brief fancy stuff here control loops and/or filters
 * Called every 50ms;
 */
void run_filters_and_control()
{
	switch (systemState)
	{
		case PanicMode:
			processPanicMode();
			break;
		case SafeMode:
			motors_off();
			return;
		case ManualMode:
			processManualMode();
			break;
		case YawControlledMode:
			processYawControl();
			break;
		case FullControllMode:
		case WirelessControl: //I assumed wireless is full control without a cable :)
			processFullControl();
			break;
		case HeightControl:
			processHeightControl();
			break;
		case RawMode:
			processRawMode();
			break;
		default:
			break;
	}

	//Process keyboard actions:
	if (systemCounter >= (lastKeyProcess + KEYDEBOUNCE)) {
		processBasicKeyPress(systemState);
	}

	update_motors();
}

/**
 * @brief Calculate the current height of the Drone. 
 * TODO: This is not used right now?
 * 
 * @param temp 
 * @param pressure 
 * @return uint16_t 
 */
uint16_t calculateHeight(int32_t temp, int32_t pressure) {
	return 1;
	//return ((pow((cData.seaLevelPressure/pressure), 1/5.257)-1)*(temp+273.15))/0.0065;
}


/**
 * @brief Integer square root (using binary search)
 *  source: https://en.wikipedia.org/wiki/Integer_square_root#Algorithm_using_binary_search
 * 
 * @author Philip Groet
 */
unsigned int isqrt( unsigned int y )
{
	startProfiling(p_Sqrt);

	unsigned int L = 0;
	unsigned int M;
	unsigned int R = y + 1;

	uint16_t i = 0;

  while( L != R - 1 ) {
		i++;

		if (i > 40) {
			packMessage(DEBUG, NULL, "ERR: isqrt too much");
			if(setSystemState(SafeMode))
			{
				// Send ACK
				uint8_t ack = SafeMode;
				packMessage(ACK, &ack, NULL);
			}
		}

    	M = (L + R) / 2;

		if( M * M <= y ) {
			L = M;
		}
		else {
			R = M;
		}
	}

	stopProfiling(p_Sqrt, true, &profileData);


	//TODO save to log?
	//Well maybe make a buffer that can be saved when full, because else it would slow down the sqrt function. 

  return L;
}

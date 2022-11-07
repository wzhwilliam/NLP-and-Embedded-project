/*------------------------------------------------------------------
 *  in4073.c -- test QR engines and sensors
 *
 *  reads ae[0-3] uart rx queue
 *  (q,w,e,r increment, a,s,d,f decrement)
 *
 *  prints timestamp, ae[0-3], sensors to uart tx queue
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  June 2016
 *------------------------------------------------------------------
 */
#include "in4073.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "adc.h"
#include "barometer.h"
#include "gpio.h"
#include "spi_flash.h"
#include "timers.h"
#include "twi.h"
#include "hal/uart.h"
#include "control.h"
#include "mpu6050/mpu6050.h"
#include "utils/quad_ble.h"
#include "utils/tools.h"
#include "comm.h"   

#define RequiredBatterySamples 20

profilingData profileData;

// Current system state. Do not write to this directly, use bool setSystemState(enum SystemState_t newState)
enum SystemState_t systemState;
// Incremented every 50ms
uint32_t systemCounter = 0;

// Trim values of th accelerometer sensor. To be set in CalibrationMode
int16_t phi_trim, psi_trim, theta_trim;
int32_t _phi_trim, _psi_trim, _theta_trim;
// Trim values of th gyroscope sensor. To be set in CalibrationMode
int16_t sr_trim, sp_trim, sq_trim;
int32_t _sr_trim, _sp_trim, _sq_trim;

//For battery average
uint16_t batteryTotal = 0;
uint8_t batterySampleCount = 0;

//For pressure average:
int32_t pressureCache[10];

// Change to true, when we want to finish our fly and send the log
bool flyEnded = false;
bool useDmp = true;

/**
 * @brief Execute any action that needs to be performed before the mode change occurs:
 * 
 * @author Wesley de Hek
 * @param newState - The new system state
 */
void specificActionsNewState(enum SystemState_t newState) {
	//Checking newState on switch:
	switch(newState) {
		case HeightControl:	
			//Setting goal pressure and saving current throttle:
			pressure_pre = pressure;
			//pressure_pre = calculateAverage(pressureCache, 10);
			throttle_pre = (int16_t)joystickThrottle;
			char msg[100];
			snprintf(msg, 100, "Height control, pressure_pre: %ld, throttle_pre: %d", pressure_pre, throttle_pre);
			packMessage(DEBUG, NULL, msg);
			// switch (light)
			// {
			// case 0:
			// 	nrf_gpio_pin_set(RED);
			// 	nrf_gpio_pin_set(GREEN);
			// 	nrf_gpio_pin_toggle(YELLOW);
			// 	break;
			// case 1:
			// 	nrf_gpio_pin_set(YELLOW);
			// 	nrf_gpio_pin_set(GREEN);
			// 	nrf_gpio_pin_toggle(RED);
			// 	break;
			// case 2:
			// 	nrf_gpio_pin_toggle(RED);
			// 	nrf_gpio_pin_toggle(YELLOW);
			// 	nrf_gpio_pin_toggle(GREEN);
			// 	break;
			// }
			break;	
		default:
			break;
	}

	//Turning on DMP based on mode:
	if(newState == RawMode && useDmp) {
		useDmp = false;
		packMessage(DEBUG, NULL, "Turning dmp off.");

		imu_init(false, 100);
	} 
	else if(newState != CalibrationMode && newState != RawMode && !useDmp) {
		useDmp = true;
		packMessage(DEBUG, NULL, "Turning dmp on.");

		imu_init(true, 100);
	}
}

/**
 * @brief Set the System State object, checking if the transition is allowed
 * 
 * @author all?
 * @return true State transition succeeded
 * @return false new state no allowed
 */
bool setSystemState(enum SystemState_t newState) {

	// Array for logging the mode changes
	uint8_t mcData[TELEM_SIZE];
	mcData[0] = systemState;
	mcData[1] = newState;

	//Switching from panic can only happen to safe mode:
	if(systemState == PanicMode && newState != SafeMode) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Can't change the mode to %d, panic mode only allows transition to safe mode.", newState);
		packMessage(DEBUG, NULL, msg);				// Log failed change
		
		// Log failed change
		mcData[2] = 0;
		saveLog(ModeChg, mcData);

		return false;
	}

	//Only allow switching to height control from full control:
	if(newState == HeightControl && systemState != FullControllMode) {
		packMessage(DEBUG, NULL, "Can't switch to Height control mode when not in Full control mode.");
		// Log failed change
		mcData[2] = 0;
		saveLog(ModeChg, mcData);

		return false;
	}

	//Checking old state on switch:
	switch (systemState)
	{
		case SafeMode:
			// Can't start the drone if joystick is not in 0 position
			if ((newState != SafeMode) && ((joystickRoll != 127) || (joystickPitch != 127) || (joystickYaw != 127) || (joystickThrottle != 0)))
			{
				//Printing reason to console:
				char msg[100];
				snprintf(msg, sizeof(msg), "Can't change the mode, joystick not neutral, roll: %d, pitch: %d, yaw: %d, throttle: %d", joystickRoll, joystickPitch, joystickYaw, joystickThrottle);
				packMessage(DEBUG, NULL, msg);
				
				// Log failed change
				mcData[2] = 0;
				saveLog(ModeChg, mcData);
				return false;
			}

			break;
		case CalibrationMode:
		{
			//Showing calibration values in console:
			char msg[100];
			snprintf(msg, sizeof(msg), "Calibration done. Using sr_trim=%d sp_trim=%d sq_trim=%d, phi_trim=%d psi_trim=%d theta_trim=%d", sr_trim, sp_trim, sq_trim, phi_trim, psi_trim, theta_trim);
			packMessage(DEBUG, NULL, msg);

			break;
		}
	default:
		break;
	}

	// Don't allow the change to mode 2,3,4,5,6,8 if motors are running
	if (newState > PanicMode  // if new state is active mode
		&& (motor[0] != 0 || motor[1] != 0 || motor[2] != 0 || motor[3] != 0) // only allow wghen motors off
		&& !(systemState == FullControllMode && newState == HeightControl) //Allowing mode switch from FULL -> Height
		&& !(systemState == HeightControl && newState == FullControllMode) //Allowing mode switch from Height -> FULL
	) {
		packMessage(DEBUG, NULL, "Can't change the mode, motor not off");
		// Log failed change
		mcData[2] = 0;
		saveLog(ModeChg, mcData);
		return false;
	}
	
	//Process any action that need to happen before transitioning to the new state:
	specificActionsNewState(newState);

	systemState = newState;

	//Showing mode change successfull in console:
	char msg[100];
	snprintf(msg, sizeof(msg), "Mode successfully changed to: %d", newState);
	packMessage(DEBUG, NULL, msg);

	// Log successful change
	mcData[2] = 1;
	saveLog(ModeChg, mcData);

	return true;
}

/**
 * @brief Function to set flyEnded variable true. It exits from the main
 * loop and stops the drone program. We will get back to bootloader after
 * sending the logged data.
 * @author Kristóf
 */
void finishFlying()
{
	flyEnded = true;
}

/**
 * @brief Check the current battery voltage and send message/put it in panic mode.  
 * It collects the battery values and averages it.
 * 
 * @author Wesley de Hek
 * @param battery_voltage - Current battery voltage.
 */
void checkBatteryStatus(uint16_t battery_voltage) {
	//Save battery value:
	batteryTotal += bat_volt;
	batterySampleCount++;

	//When enough samples are collected, calculate average and check status:
	if(batterySampleCount >= RequiredBatterySamples) {
		uint16_t averageBatteryVoltage = batteryTotal / batterySampleCount;

		//Reset collection:
		batteryTotal = 0;
		batterySampleCount = 0;

		//Battery empty warning: https://blog.ampow.com/lipo-voltage-chart/, only used if battery not usb and battery voltage in warning range
		if(averageBatteryVoltage > 650 && averageBatteryVoltage <= 1120) { // 12.622V is read as 12.320 -> offset in adc.c
			char msg[100];
			snprintf(msg, 100, "Battery voltage low: %d volt", averageBatteryVoltage);
			packMessage(DEBUG, NULL, msg);
		}

		switch(systemState) {
			case ManualMode:
			case CalibrationMode:
			case YawControlledMode:
			case FullControllMode:
			case RawMode:
			case HeightControl:
			case WirelessControl:
				// If battery not usb and battery voltage is starting to damage the battery. Never below 10.5 volts
				if(averageBatteryVoltage > 650 && averageBatteryVoltage <= 1050) {
					char msg[100];
					snprintf(msg, 100, "Battery empty, putting drone in panic mode (%d volt)", averageBatteryVoltage);
					packMessage(DEBUG, NULL, msg);

					if(setSystemState(PanicMode))
					{
						// Send ACK
						uint8_t ack = PanicMode;
						packMessage(ACK, &ack, NULL);
					}
				}
				break;
			default:
				break;
		}
	}
}

/**
 * @brief Checks if connection was lost and puts drone in panic mode if it is lost.
 * 
 * @author Wesley de Hek
 */
void connectionLostCheck() {
	if(!checkConnection() && systemState != PanicMode && systemState != SafeMode) {
		packMessage(DEBUG, NULL, "Connection lost, putting drone in panic mode!");
		if(setSystemState(PanicMode))
		{
			// Send ACK
			uint8_t ack = PanicMode;
			packMessage(ACK, &ack, NULL);
		}
	}
}

/**
 * @brief Save the current average profiling values per type to the log file.
 * TODO: Making it work.
 * @author Wesley de Hek
 */
void saveProfilingResults(profilingTelemetry *telem) {
	uint32_t averageResponseTime = 0;
	uint8_t profileDataToLog[TELEM_SIZE];

	//Grabbing control loop time:
	averageResponseTime = getAverageProfilingTime(p_ControlLoop, &profileData);
	telem->controlLoopTime = averageResponseTime;

	//Grabbing timer flag time:
	averageResponseTime = getAverageProfilingTime(p_Timer_Flag, &profileData);
	telem->timerFlagTime = averageResponseTime;

	//Grabbing yaw mode time:
	averageResponseTime = getAverageProfilingTime(p_YawMode, &profileData);
	telem->yawModeTime = averageResponseTime;

	//Grabbing full mode time:
	averageResponseTime = getAverageProfilingTime(p_FullControl, &profileData);
	telem->fullModeTime = averageResponseTime;

	//Grabbing raw mode time:
	averageResponseTime = getAverageProfilingTime(p_RawMode, &profileData);
	telem->rawModeTime = averageResponseTime;

	//Grabbing height mode time:
	averageResponseTime = getAverageProfilingTime(p_HeightMode, &profileData);
	telem->heightModeTime = averageResponseTime;

	//Grabbing logging time:
	averageResponseTime = getAverageProfilingTime(p_Logging, &profileData);
	telem->loggingTime = averageResponseTime;

	//Grabbing sqrt time:
	averageResponseTime = getAverageProfilingTime(p_Sqrt, &profileData);
	telem->sqrtTime = averageResponseTime;

	//Serializing data:
	serializeProfiling(telem, profileDataToLog);

	//Logging progiling data:
	saveLog(l_Profiling, profileDataToLog);

	//sendProfilingData(telem);
}

/**
 * @brief Apply calibration to the different sensors, runs it 100 times and than switches back to safe mode.
 * Calibrates: sr, sp, sq, phi, psi, theta
 * 
 * @author Philip Groet
 */
void processCalibration() {
	if(cData.calibrationCycle == 0) {
		char msg[100];
		snprintf(msg, 100, "Calibration started! (Mode: %s)", useDmp ? "DMP" : "RAW");
		packMessage(DEBUG, NULL, msg);
	}

	if(cData.calibrationCycle < CALIBRATION_SAMPLES) {
		// _sr_trim is 10.000 times magnitude of sr
		// Have the old values weigh for 9/10 and the new value 1/10
		_sr_trim = (_sr_trim*9 + sr*10000) / 10;
		// Restore magnitude from 10.000 to 1
		sr_trim = _sr_trim / 10000;

		_sp_trim = (_sp_trim*9 + sp*10000) / 10;
		sp_trim = _sp_trim / 10000;

		_sq_trim = (_sq_trim*9 + sq*10000) / 10;
		sq_trim = _sq_trim / 10000;


		_phi_trim = (_phi_trim*9 + phi*10000) / 10;
		phi_trim = _phi_trim / 10000;

		_psi_trim = (_psi_trim*9 + psi*10000) / 10;
		psi_trim = _psi_trim / 10000;

		_theta_trim = (_theta_trim*9 + theta*10000) / 10;
		theta_trim = _theta_trim / 10000;

		cData.calibrationCycle++;
	}
	else {
		//Putting drone in safe mode:
		if(setSystemState(SafeMode))
		{
			// Send ACK
			uint8_t ack = SafeMode;
			packMessage(ACK, &ack, NULL);
		}

		cData.calibrationCycle = 0;
	}
}


/**
 * @brief The soul of our lovely drone :)
 * @author Everyone :)
 */
int main(void)
{
	uart_init();
	gpio_init();
	timers_init();
	adc_init();
	twi_init();
	imu_init(true, 100);
	baro_init();
	spi_flash_init();
	quad_ble_init();
	comm_init();

	systemState = SafeMode;

	//Initialize motor control, sets motors to 0;
	initializeMotorControl();

	uint32_t panicStart = 0;

	recMachine SSM;
	SSM.actualState = START;
	recMachine BSM;
	BSM.actualState = START;

	telemetry telem;
	uint8_t telemData[TELEM_SIZE];

	profilingTelemetry profilingTelem;

	uint8_t maxRead;

	//Initializing profiling data:
	initProfiling(&profileData);

	while (!flyEnded) { //We should be able to break out of this, else the log will never be send.
		maxRead = 0;

		//Reading message queue
		while ((maxRead < 10) && rx_queue.count) {
			unpackMessage(dequeue(&rx_queue), &SSM);
			maxRead++;
		}
		maxRead = 0;

		//Reading bluetooth queue
		while ((maxRead < 10) && ble_rx_queue.count) {
			uint8_t rec = dequeue(&ble_rx_queue);
			unpackMessage(rec, &BSM);
			maxRead++;
		}

		// Every 50ms
		if (check_timer_flag()) {
			startProfiling(p_Timer_Flag);

			//Checking if connection didn't get broken:
			connectionLostCheck();

			//Blink yellow light in panic mode:
			if (systemState == PanicMode && systemCounter%2 == 0) {
				nrf_gpio_pin_toggle(YELLOW);
			}

			// Save current telemetry to log
			telem.mode = systemState;
			telem.motor1 = motor[0]; telem.motor2 = motor[1]; telem.motor3 = motor[2]; telem.motor4 = motor[3];
			telem.phi = phi-phi_trim; telem.theta = theta-theta_trim; telem.psi = psi-psi_trim;
			telem.sp = sp - sp_trim; telem.sq = sq - sq_trim; telem.sr = sr - sr_trim;
			telem.bat = bat_volt; telem.temp = temperature; telem.pres = pressure;
			telem.p = Gain_Yaw; telem.p1 = Gain_P1; telem.p2 = Gain_P2; telem.hei = Gain_height;
			serializeTelemetry(&telem, telemData);
			saveLog(Telemetry, telemData);

			//Check battery voltage status:
			checkBatteryStatus(bat_volt);

			//Save progiling data to log:
			//saveProfilingResults(&profilingTelem);

			// Send telemetry every second
			if (systemCounter % 20 == 0) sendTelemetry(telemData);

			// Every 20 50ms periods = Every second
			if (systemCounter%20 == 0) {
				//Blinking blue led to show drone is still alive:
				nrf_gpio_pin_toggle(BLUE);

				// Send telemetry every 1 sec -> for debug
				//sendTelemetry(telemData);

				//Send profiling data to terminal:
				//sendProfilingData(&profilingTelem);
			}

			adc_request_sample();
			read_baro();	

			//Saving pressure for height control:
			pressureCache[systemCounter%10] = pressure;		

			clear_timer_flag();

			stopProfiling(p_Timer_Flag, true, &profileData);
		}

		if (check_sensor_int_flag()) {
			startProfiling(p_ControlLoop);			

			get_sensor_data();

			//Run calibration when in calibration mode:
			if (systemState == CalibrationMode) {
				processCalibration();
			}

			run_filters_and_control();

			stopProfiling(p_ControlLoop, true, &profileData);
		}

		//Turn drone into safe mode when pressing the escape key:
		if (systemState != SafeMode && keys[ESC_KEY]) {
			packMessage(DEBUG, NULL, "ESC pressed, going to safemode");

			if(setSystemState(PanicMode))
			{
				// Send ACK
				uint8_t ack = PanicMode;
				packMessage(ACK, &ack, NULL);
			}
		}

		//Checking current system state:
		switch (systemState)
		{
			case PanicMode:
				if (!panicStart) {
					panicStart = global_time;
				}

				// Stay in panic mode for 3 seconds, then transition to safemode
				if ((global_time - panicStart) > 3 * 1000000) {
					panicStart = 0;
					if(setSystemState(SafeMode))
					{
						// Send ACK
						uint8_t  ack = SafeMode;
						packMessage(ACK, &ack, NULL);
					}
				}
				break;
			case ManualMode:
			case CalibrationMode:
			case YawControlledMode:
			case FullControllMode:
			case RawMode:
			case HeightControl:
			case SafeMode:
			case WirelessControl:
				break;

			default:
				packMessage(DEBUG, NULL, "ERR: Unknown system state");
				if(setSystemState(PanicMode))
				{
					// Send ACK
					uint8_t  ack = PanicMode;
					packMessage(ACK, &ack, NULL);
				}
				break;
		}
	}

	// Send log in the end
	sendLog();

	packMessage(DEBUG, NULL, "Fly ended. Goodbye! :)");

	// Send ACK of finished flying
	uint8_t ack = '.';
	packMessage(ACK, &ack, NULL);

	nrf_delay_ms(100);

	NVIC_SystemReset();
}

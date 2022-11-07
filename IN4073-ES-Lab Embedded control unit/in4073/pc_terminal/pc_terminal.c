/*------------------------------------------------------------
 * Simple pc terminal in C
 *
 * Arjan J.C. van Gemund (+ mods by Ioannis Protonotarios)
 *
 * read more: http://mirror.datenwolf.net/serial/
 *------------------------------------------------------------
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h> 

// Includes for communication
#include "../communication/protocol.h"
#include "../communication/joy.h"
#include "../communication/config.h"

// Timer for periodic stuff
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

/*----------------------------------------------------------------
 * main -- execute terminal
 *----------------------------------------------------------------
 */

/**
 * @brief The main program for the PC side. Handles PC-drone
 * communication, processes keyboard inputs.
 * @author Kristóf
 */
int main(int argc, char **argv)
{
	int res;
	char c;
	bool joystickFound = true;

	// Message fields
	uint8_t config[CFG_SIZE];
	uint8_t pilotCmd[CMD_SIZE];
	pilotCmd[0] = 0;	// Keys 0
	pilotCmd[1] = 0;	// Keys 0
	pilotCmd[2] = 127;	// Joy roll middle
	pilotCmd[3] = 127;	// Joy pitch middle
	pilotCmd[4] = 127;	// Joy yaw middle
	pilotCmd[5] = 0;	// Joy throttle 0
	pilotCmd[6] = 0;	// Keys 0

	// State machine for receiving by uart
	recMachine SSM;
	SSM.actualState = START;

	// State machine for receiving by bluetooth
	recMachine BSM;
	BSM.actualState = START;

	term_initio();
	term_puts("\nTerminal program - Embedded Real-Time Systems\n");

	// If no argument is given at execution time, /dev/ttyUSB0 is assumed
	// Asserts are in the function
	if (argc == 1) {
		serial_port_open(SERIAL_PORT);
	} else if (argc == 2) {
		serial_port_open(argv[1]);
	} else {
		printf("Wrong number of arguments\n");
		return -1;
	}

	// Open joystick
	if(openJoy())
	{
		printf("Joystick not found, disabling joystick controll\n");
		joystickFound = false;

		if(!DEBUG_MODE) {
			return -1;
		}
	}

	term_puts("Type ^C to exit\n");

	// Send config at the begin
	// Fill the config array
	int16_t gains[4] = {P_VALUE, P1_VALUE, P2_VALUE, HEIGHT_GAIN};
	i16_to_ui8(gains[0], &config[0]);
    i16_to_ui8(gains[1], &config[2]);
    i16_to_ui8(gains[2], &config[4]);
    i16_to_ui8(gains[3], &config[6]);
	packMessage(CFG, config);

	// Open TCP socket -> for processing
	openSocket();

	// Set before time
	struct timeval tval_before, tval_after, tval_result;
	gettimeofday(&tval_before, NULL);

	bool finished = false;
	while (!finished) 
	{
		// Read characters and act
		if ((c = term_getchar_nb()) != -1)
		{
			processKeyboard(c, pilotCmd);
		}

		// Get current time to compare time before
		gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = ((double)tval_result.tv_sec*1000.0f) + ((double)tval_result.tv_usec/1000.0f);

		// Pack and send the messages
		if (time_elapsed > SEND_RATE_MS)
		{
			// Read joystick axes and buttons
			if(joystickFound) {
				getJoyValues(pilotCmd);
			}
			
			// Pack message
			packMessage(CMD, pilotCmd);
			// Don't reset the joystick axes (2, 3, 4, 5) !!!
			pilotCmd[0] = 0;
			pilotCmd[1] = 0;
			pilotCmd[6] = 0;

			// Store new before time
			gettimeofday(&tval_before, NULL);
		}

        // Process received characters -> both serial and bluetooth
        // With the while loop we can read multiple chars in one cycle
		int ret;
		while ((res = serial_port_getchar(UART)) != -1)
		{
			c = (char)res;
			ret = unpackMessage(c, &SSM);
			if (ret == '.') finished = true;
		}
		while ((res = serial_port_getchar(BLUETOOTH)) != -1)
		{
			c = (char)res;
			ret = unpackMessage(c, &BSM);
			if (ret == '.') finished = true;
		}
	}

    // Close the serial and bluetooth port
	serial_port_close();
	ble_port_close();
	printf("Ports closed\n");
	
	// Close TCP socket
	closeSocket();

	term_exitio();
	term_puts("\n<exit>\n");
	return 0;
}


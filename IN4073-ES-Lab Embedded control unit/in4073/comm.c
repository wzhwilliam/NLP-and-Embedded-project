#include "comm.h"
#include "utils/profiling.h"
#include "hal/timers.h"
#include "hal/spi_flash.h"
#include "hal/uart.h"

// Array to store pressed keys
bool keys[18];

// Flash memory variables
uint32_t flashAdr = 0x000000;
uint32_t storedRows = 0;

// Joystick values
uint8_t joystickRoll;
uint8_t joystickPitch;
uint8_t joystickYaw;
uint8_t joystickThrottle;

// Time of last received message
uint32_t last_system_count;

char msg[100];

void comm_init()
{
	memset(keys, 0, sizeof(keys));
}

/**
 * @brief Functions to serialize / deserialize 2/4 bytes wide variables
 * @author Kristóf
 */
void ui16_to_ui8(uint16_t source, uint8_t *dest) 
{ 
	dest[0] = (uint8_t)(source >> 8);
	dest[1] = (uint8_t)(source & 0xFF);
}

void i16_to_ui8(int16_t source, uint8_t *dest) 
{
	dest[0] = (uint8_t)(source >> 8);
	dest[1] = (uint8_t)(source & 0xFF);
}
void ui32_to_ui8(uint32_t source, uint8_t *dest) 
{
	dest[0] = (uint8_t)(source >> 24);
	dest[1] = (uint8_t)((source >> 16) & 0xFF);
	dest[2] = (uint8_t)((source >> 8) & 0xFF);
	dest[3] = (uint8_t)(source & 0xFF);
}
void i32_to_ui8(int32_t source, uint8_t *dest)
{
	dest[0] = (uint8_t)(source >> 24);
	dest[1] = (uint8_t)((source >> 16) & 0xFF);
	dest[2] = (uint8_t)((source >> 8) & 0xFF);
	dest[3] = (uint8_t)(source & 0xFF);
}

uint16_t to_ui16(uint8_t *pData) 
{ 
	uint16_t ret = 0;
	ret |= (pData[0] << 8);
	ret |= pData[1];
	return ret;
}
int16_t to_i16(uint8_t *pData) 
{
	int16_t ret = 0;
	ret |= (pData[0] << 8);
	ret |= pData[1]; 
	return ret;
}
uint32_t to_ui32(uint8_t *pData) 
{
	uint32_t ret = 0;
	ret |= (pData[0] << 24);
	ret |= (pData[1] << 16);
	ret |= (pData[2] << 8);
	ret |= pData[3];
	return ret;
}
int32_t to_i32(uint8_t *pData) 
{ 
	int32_t ret = 0;
	ret |= (pData[0] << 24);
	ret |= (pData[1] << 16);
	ret |= (pData[2] << 8);
	ret |= pData[3];
	return ret;
}


/**
 * @brief Function to process the received message
 * @param msgType Message type enum
 * @param uint8_t* Pointer to data to process
 * @author Kristóf
 */
void processMsg(msgType type, uint8_t *pData)
{
	// Array for logging the commands
	uint8_t cmdData[TELEM_SIZE];

	// Incremental index, used at storing log of CMD
	static uint32_t idx = 0;

	switch (type)
	{
	case CFG:
	{
		// Modify configurable variables
		Gain_Yaw = to_i16(&pData[0]);
		Gain_P1 = to_i16(&pData[2]);
		Gain_P2 = to_i16(&pData[4]);
		Gain_height = to_i16(&pData[6]);
		// Send one character ACK after config
		uint8_t ack = 'C';
		packMessage(ACK, &ack, NULL);
		break;
	}

	case MODE:
	{
		packMessage(DEBUG, NULL, "RX Mode change");

		// Call change mode function
		if(setSystemState(pData[0]))
		{
			// Send ACK
			// Wireless -> first send ACK, then change
			if (pData[0] == 8)
			{
				uint8_t  ack = pData[0];
				packMessage(ACK, &ack, NULL);
				wireless_mode = true;
			}
			// Back from wireless -> first change, then send ACK
			else
			{
				wireless_mode = false;
				uint8_t  ack = pData[0];
				packMessage(ACK, &ack, NULL);
			}
		}
		break;
	}

	case CMD:
	{
		// Set keys
		for (uint8_t i = 0; i < 8; i++)
		{
			keys[i] = ((pData[0] >> (7-i)) & 0x01);
		}
		for (uint8_t i = 0; i < 8; i++)
		{
			keys[i+8] = ((pData[1] >> (7-i)) & 0x01);
		}
		keys[16] = ((pData[6] >> 1) & 0x01);
		keys[17] = (pData[6] & 0x01);

		if (keys[JOY_FIRE] == 1)
		{
			packMessage(DEBUG, NULL, "Joystick fire -> panic");
			if(setSystemState(PanicMode))
			{
				// Send ACK
				uint8_t  ack = PanicMode;
				packMessage(ACK, &ack, NULL);
			}
		}
		
		// Update joystick values
		joystickRoll = pData[2];
		joystickPitch = pData[3];
		joystickYaw = pData[4];
		joystickThrottle = pData[5];

		// Save CMD to log only every 5th time or when a key is pressed
		if ((idx % 5 == 0) || pData[0] || pData[1] || pData[6])
		{
			for (uint8_t i = 0; i < CMD_SIZE; i++)
			{
				cmdData[i] = pData[i];
			}
			saveLog(Command, cmdData); 
		}
		idx++;

		// End flight if user pressed '.' (only in safe mode)
		if (((pData[6] >> 7) == 1) && (systemState == SafeMode))
		{
			packMessage(DEBUG, NULL, "Finish flying");
			finishFlying();
		}
		break;
	}
	
	default:
	{
		// Go to panic mode if unknown command arrived
		packMessage(DEBUG, NULL, "Received unknown command");
		if(setSystemState(PanicMode))
		{
			// Send ACK
			uint8_t ack = PanicMode;
			packMessage(ACK, &ack, NULL);
		}
		break;
	}
	}
}

/**
 * @brief Function to pack and sends the messages based on their type
 * @param msgType Message type enum
 * @param uint8_t* Pointer to data to send
 * @param char* Debug message as character string
 * @author Kristóf
 */
void packMessage(msgType type, uint8_t *pData, const char* debugMsg)
{
	// Send log in blocking mode -> other way the queue would get full almost immediately
	bool blocking = false;
	if (type == LOG) blocking = true;

	// DON'T send any messages in wireless mode!!!
	if (!wireless_mode)
	{	
		uint8_t checkSum = 0;

		// Start character
		uart_put((uint8_t)'?', blocking);
		checkSum ^= '?';

		// Message type
		uart_put((uint8_t)type, blocking);
		checkSum ^= type;

		// Add the length and the data to the message
		switch (type)
		{
		case TELEM:
			uart_put((uint8_t)TELEM_SIZE, blocking);
			checkSum ^= TELEM_SIZE;
			for (uint8_t i = 0; i < TELEM_SIZE; i++)
			{
				uart_put(pData[i], blocking);
				checkSum ^= pData[i];
			}
			break;
			
		case LOG:
			uart_put((uint8_t)LOG_SIZE, blocking);
			checkSum ^= LOG_SIZE;
			for (uint8_t i = 0; i < LOG_SIZE; i++)
			{
				uart_put(pData[i], blocking);
				checkSum ^= pData[i];
			}
			break;
			
		case ACK:
			uart_put((uint8_t)1, blocking);
			checkSum ^= 1;
			uart_put((uint8_t)pData[0], blocking);
			checkSum ^= pData[0];
			break;

		case DEBUG:
			// We can send debug messages (there's no length and checksum then, always ends with the '\n' character)
			printf("%s\n", debugMsg);
			break;

		default:
			uart_put((uint8_t)1, blocking);	// Length 1
			checkSum ^= 1;
			uart_put((uint8_t)0, blocking);	// Emergeny stop data = 0
			checkSum ^= 0;
			break;
		}

		// Last field of message is the checksum (except DEBUG)
		if(type != DEBUG) uart_put(checkSum, blocking);
	}
}

/**
 * @brief Function to process incoming characters.
 * Calls processMsg, when a message is finished. 
 * @param uint8_t Received character
 * @param recMachine Pointer to receiving state machine
 * @author Kristóf
 */
void unpackMessage(uint8_t c, recMachine *SM)
{
	uint8_t error = 0;

	//Save time of last received byte:
	last_system_count = systemCounter;

	// State machine to process incoming message byte-to-byte
    switch (SM->actualState)
	{
	// Step next state if start character received
	case START:
		if (c == '?')
		{
			SM->recCsum = 0;
			SM->idx = 0;
			SM->actualState++;
		}
		break;

	case TYPE:
		SM->recType = c;
		if (SM->recType != CFG && SM->recType != MODE && SM->recType != CMD)
		{
			error = 1; // Start again as we have an error
			packMessage(DEBUG, NULL, "DRONE: Type error at receiving!");
		}
		SM->actualState++;
		break;

	case LENGTH:
		SM->recLength = c;
		SM->actualState++;
		break;

	case DATA:
		// Read data bits (length times)
		SM->msg[SM->idx++] = c;
		SM->recLength--;
		if (!SM->recLength)
			SM->actualState++;
		break;

	case CSUM:
		if (c != SM->recCsum)
		{
			packMessage(DEBUG, NULL, "DRONE: Checksum error at receiving!");
		}
		// Process message if check sum was correct
		else
		{
			processMsg(SM->recType, SM->msg);
		}
		SM->actualState = START;
		break;
	
	default:
		// Reset state machine
		packMessage(DEBUG, NULL, "DRONE: Error at receiving!");
		SM->actualState = START;
		break;
	}

	// Add received character to checksum
	SM->recCsum ^= c;

	// If there was an error reset state machine and wait for new messages
	if (error)
	{
		SM->actualState = START;
	}
	
}

/**
 * @brief Function to serialize telemetry data into an uint8_t array
 * @param telemetry* Pointer to telemetry struct
 * @param uint8_t* Pointer to array where serialized telemetry data starts
 * @author Kristóf
 */
void serializeTelemetry(telemetry *telem, uint8_t *pData)
{
	pData[0] = telem->mode;
	i16_to_ui8(telem->motor1, &pData[1]);
	i16_to_ui8(telem->motor2, &pData[3]);
	i16_to_ui8(telem->motor3, &pData[5]);
	i16_to_ui8(telem->motor4, &pData[7]);
	i16_to_ui8(telem->phi, &pData[9]);
	i16_to_ui8(telem->theta, &pData[11]);
	i16_to_ui8(telem->psi, &pData[13]);
	i16_to_ui8(telem->sp, &pData[15]);
	i16_to_ui8(telem->sq, &pData[17]);
	i16_to_ui8(telem->sr, &pData[19]);
	ui16_to_ui8(telem->bat, &pData[21]);
	i32_to_ui8(telem->temp, &pData[23]);
	i32_to_ui8(telem->pres, &pData[27]);
	i16_to_ui8(telem->p, &pData[31]);
	i16_to_ui8(telem->p1, &pData[33]);
	i16_to_ui8(telem->p2, &pData[35]);
	i16_to_ui8(telem->hei, &pData[37]);
}

/**
 * @brief Function to serialize the profiling data into an uint8_t array
 * 
 * @author Wesley de Hek
 * @param profilingTelem - The profiling data 
 * @param pData - The array to store the data in
 */
void serializeProfiling(profilingTelemetry *profilingTelem, uint8_t *pData) {
	ui32_to_ui8(profilingTelem->controlLoopTime, &pData[0]);
	ui32_to_ui8(profilingTelem->timerFlagTime, &pData[4]);
	ui32_to_ui8(profilingTelem->yawModeTime, &pData[8]);
	ui32_to_ui8(profilingTelem->fullModeTime, &pData[12]);
	ui32_to_ui8(profilingTelem->rawModeTime, &pData[16]);
	ui32_to_ui8(profilingTelem->heightModeTime, &pData[20]);
	ui32_to_ui8(profilingTelem->loggingTime, &pData[24]);
	ui32_to_ui8(profilingTelem->sqrtTime, &pData[28]);
}

/**
 * @brief Writes the profiling data as debug to console:
 * 
 * @author Wesley de Hek
 * @param profilingTelem - Profiling telemetry
 */
void sendProfilingData(profilingTelemetry *profilingTelem) {
	char msg[100];
	snprintf(msg, 100, "Profiling: Control (%ld ns), Timer (%ld ns), Yaw (%ld ns), Full (%ld ns), Raw (%ld ns), Height (%ld ns) ,Log (%ld ns), Sqrt (%ld ns)", 
		profilingTelem->controlLoopTime,
		profilingTelem->timerFlagTime,
		profilingTelem->yawModeTime,
		profilingTelem->fullModeTime,
		profilingTelem->rawModeTime,
		profilingTelem->heightModeTime,
		profilingTelem->loggingTime,
		profilingTelem->sqrtTime);
	packMessage(DEBUG, NULL, msg);
}

/**
 * @brief Function to send all telemetry data to PC
 * @param uint8_t* Pointer where serialized telemetry data starts
 * @author Kristóf
 */
void sendTelemetry(uint8_t *telemData)
{
	packMessage(TELEM, telemData, NULL);
}

/**
 * @brief Function to save log to flash.
 * Maybe add length parameter, so that I can also log the profiling results :)
 * @param logType Log type enum
 * @param uint8_t* Pointer to data which we want to save in flash memory (max. TELEM_SIZE bytes)
 * @author Kristóf
 */
void saveLog(logType type, uint8_t *pData)
{
	// Set to true when flash got full
	static bool flashFull = false;

	// Don't store more data when flash is full
	if (!flashFull)
	{
		startProfiling(p_Logging);

		// Check if there is still free space in the flash
		// 131071 bytes -> for 44 bytes packages it's only 2978 rows
		if (flashAdr >= (131070 - 3 * (LOG_SIZE)))
		{
			flashFull = true;
			packMessage(DEBUG, NULL, "Flash is full, storing last row");
		}

		// Store current time in flash
		uint8_t ts[4];
		ui32_to_ui8(get_time_us(), ts);
		if(!flash_write_bytes(flashAdr, ts, 4)) packMessage(DEBUG, NULL, "Flash write error");
		flashAdr += 4;	

		// Store log type
		if(!flash_write_byte(flashAdr, type)) packMessage(DEBUG, NULL, "Flash write error");
		flashAdr += 1;

		// Store data in flash
		if(!flash_write_bytes(flashAdr, pData, TELEM_SIZE))  packMessage(DEBUG, NULL, "Flash write error");
		flashAdr += TELEM_SIZE;

		// Increment number of rows
		storedRows++;

		// If flash got full now, we store one last row which indicates that flash is full
		if (flashFull)
		{
			// Store current time in flash
			uint8_t ts[4];
			ui32_to_ui8(get_time_us(), ts);
			if(!flash_write_bytes(flashAdr, ts, 4)) packMessage(DEBUG, NULL, "Flash write error");
			flashAdr += 4;	

			// Store log type
			if(!flash_write_byte(flashAdr, Full)) packMessage(DEBUG, NULL, "Flash write error");
			flashAdr += 1;

			storedRows++;
		}

		stopProfiling(p_Logging, true, &profileData);
	}
}

/**
 * @brief Function to send log to PC.
 * Called only when the flight is finished.
 * @author Kristóf
 */
void sendLog()
{
	packMessage(DEBUG, NULL, "Start to send log");

	uint8_t logRow[LOG_SIZE];
	uint32_t adr = 0x000000;

	snprintf(msg, 100, "Number of rows to send %lu", storedRows);
	packMessage(DEBUG, NULL, msg);
	uint32_t temp = storedRows;

	// Send all rows from the flash
	/*
	*	DON'T SEND DEBUG MESSAGES HERE, AS WE CALL THE UART_PUT IN BLOCKING MODE
	*/
	bool error = false;
	for (uint32_t i = 0; i < temp; i++)
	{
		// Read data from flash memory
		if(!flash_read_bytes(adr, logRow, LOG_SIZE)) 
		{
			error = true;
			break;
		}
		packMessage(LOG, logRow, NULL);
		adr += LOG_SIZE;
	}
	/*
	*	WE CAN SEND DEBUG MESSAGES AGAIN
	*/
	if (error == true) packMessage(DEBUG, NULL, "Flash read error");

	// Erase flash
	if(!flash_chip_erase())
	{
		packMessage(DEBUG, NULL, "Flash couldn't be erased");
	}

	// Set flash adress to 0x000000 again (we can overwrite the sent data)
	flashAdr = 0x000000;
	storedRows = 0;

	packMessage(DEBUG, NULL, "Everything sent");
}

/**
 * @brief Checks if the connection is broken, marked as broken if last command is more than 500ms ago.
 * 
 * @author Wesley de Hek
 * @return true - Connection broken.
 * @return false - Connection not broken.
 */
bool checkConnection() {
	return last_system_count >= systemCounter-10;
}
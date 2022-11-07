#include "protocol.h"

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
 * @brief 
 * 
 * console I/O functions
 * @author Part of example code
 *------------------------------------------------------------
 */
struct termios 	savetty;

void term_initio()
{
	struct termios tty;

	tcgetattr(0, &savetty);
	tcgetattr(0, &tty);

	tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	tcsetattr(0, TCSADRAIN, &tty);
}

void term_exitio()
{
	tcsetattr(0, TCSADRAIN, &savetty);
}

void term_puts(char *s)
{
	fprintf(stderr,"%s",s);
}

void term_putchar(char c)
{
	putc(c,stderr);
}

int	term_getchar_nb()
{
	static unsigned char line [2];

	if (read(0,line,1)) // note: destructive read
		return (int) line[0];

	return -1;
}

int	term_getchar()
{
	int    c;

	while ((c = term_getchar_nb()) == -1)
		;
	return c;
}

/*------------------------------------------------------------
 * Serial I/O
 * 8 bits, 1 stopbit, no parity,
 * 115,200 baud
 *------------------------------------------------------------
 */

// Uart + bluetooth file descriptors
static int fd_serial_port = -1;
static int fd_ble_port = -1;

// Bool which is true if we send data on bluetooth, else false
static bool useBluetooth = false;

/**
 * Open the terminal I/O interface to the serial/pseudo serial port.
 * @param char* Name of serial device (e.g. "/dev/ttyUSB0")
 * @author Part of example code, modified by Kristóf
 */
void serial_port_open(const char *serial_device)
{
	char *name;
	int result;
	struct termios tty;

	fd_serial_port = open(serial_device, O_RDWR | O_NOCTTY);
	printf("Serial FD %d opened\n", fd_serial_port);

	assert(fd_serial_port>=0);

	result = isatty(fd_serial_port);
	assert(result == 1);

	name = ttyname(fd_serial_port);
	assert(name != 0);

	result = tcgetattr(fd_serial_port, &tty);
	assert(result == 0);

	tty.c_iflag = IGNBRK; /* ignore break condition */
	tty.c_oflag = 0;
	tty.c_lflag = 0;

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
	tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */

	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 1; // TODO check cpu usage

	tty.c_iflag &= ~(IXON|IXOFF|IXANY);

	result = tcsetattr(fd_serial_port, TCSANOW, &tty); /* non-canonical */

	tcflush(fd_serial_port, TCIOFLUSH); /* flush I/O buffer */
}

/**
 * @brief Opens the bluetooth port as serial (almost 
 * same code as serial_port_open)
 * @param char* Name of serial device (e.g. "/dev/pts/3")
 * @author Kristóf
 */
void ble_port_open(const char *serial_device)
{
	char *name;
	int result;
	struct termios tty;

	fd_ble_port = open(serial_device, O_RDWR | O_NOCTTY);
	printf("Ble FD %d opened\n", fd_ble_port);
	if (fd_ble_port == -1)
	{
		return;
	}

	assert(fd_ble_port>=0);

	result = isatty(fd_ble_port);
	assert(result == 1);

	name = ttyname(fd_ble_port);
	assert(name != 0);

	result = tcgetattr(fd_ble_port, &tty);
	assert(result == 0);

	tty.c_iflag = IGNBRK; /* ignore break condition */
	tty.c_oflag = 0;
	tty.c_lflag = 0;

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
	tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */

	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 1; // TODO check cpu usage

	tty.c_iflag &= ~(IXON|IXOFF|IXANY);

	result = tcsetattr(fd_ble_port, TCSANOW, &tty); /* non-canonical */

	tcflush(fd_ble_port, TCIOFLUSH); /* flush I/O buffer */
}

/**
 * @brief Closes uart port.
 * @author Kristóf
 */
void serial_port_close()
{
	int result;

	// Close uart port fd
	if (fd_serial_port != -1)
	{
		result = close(fd_serial_port);
		assert (result==0);
		printf("Serial FD %d closed\n", fd_serial_port);
	}
	fd_serial_port = -1;
}

/**
 * @brief Closes bluetooth port.
 * @author Kristóf
 */
void ble_port_close()
{
	int result;

	// Close bluetooth port fd
	if (fd_ble_port != -1)
	{
		result = close(fd_ble_port);
		assert (result==0);
		printf("Ble FD %d closed\n", fd_ble_port);
	}
	fd_ble_port = -1;
}

/**
 * @brief Gets one character from a serial port (either
 * it is the uart or the bluetooth)
 * @param bool False - read uart port, True - read bluetooth port
 * @return One character if read was successful, else -1
 * @author Kristóf
 */
int serial_port_getchar(bool ble)
{
	ssize_t result = -1;
	uint8_t c;

	// Read if we use uart
	if (!ble)
	{
		result = read(fd_serial_port, &c, 1);
	}
	// Read if we use bluetooth and port is opened
	else if (fd_ble_port != -1)	
	{
		result = read(fd_ble_port, &c, 1);
	}

	if (result <= 0)
	{
		return -1;
	}
	return c;
}

/**
 * @brief Puts one character to a serial port (either
 * it is the uart or the bluetooth)
 * @param char Character to send on the serial port
 * @return 1 if write was successful, else -1
 * @author Kristóf
 */
int serial_port_putchar(char c)
{
	int result;

	do 
	{
		if (!useBluetooth) { result = (int) write(fd_serial_port, &c, 1); }
		else { result = (int) write(fd_ble_port, &c, 1); }
	}
	while (result == 0);

	// Commented out because it kills the terminal if the connection is broken. But we get -1 when we use bluetooth
	assert(result == 1);
	return result;
}

/**
 * @brief Function to get the value of bluetooth port file descriptor
 * @return Value of bluetooth port fd
 * @author Kristóf
 */
int get_fd_ble()
{
	return fd_ble_port;
}

/**
 * @brief Function to get the value of serial port file descriptor
 * @return Value of serial port fd
 * @author Kristóf
 */
int get_fd_serial()
{
	return fd_serial_port;
}

// Socket and client fd for TCP communication
int sock = -1;
int client_fd = -1;
static bool tcp_enabled = false;

/**
 * @brief Opens a new TCP socket for sending drone rotation positions and connects to Processing.org server as a client
 * @author Kristóf
 */
void openSocket()
{
  struct sockaddr_in serv_addr;

  // Create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("TCP: Socket creation error\n");
    return;
  }

  // Set IP address family + port
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(12345);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
  {
    printf("TCP: Invalid address / Address not supported\n");
    return;
  }

  // Connect to server
  if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) 
  {
    printf("TCP: Connection Failed\n");
    return;
  }

  tcp_enabled = true;
}

/**
 * @brief Closes TCP socket if it was opened
 * @author Kristóf
 */
void closeSocket()
{
	if (tcp_enabled)
	{
		if (client_fd != -1)
		{
			close(client_fd);
		}
		if (sock != -1)
		{
			close(sock);
		}
	}
	
	tcp_enabled = false;
	client_fd = -1;
	sock = -1;
}

/*----------------------------------------------------------------
 * Message protocol stuff -- pack & unpack messages
 *----------------------------------------------------------------
 */

// A few variables used at logging
static bool logFileExist = false;
char logName[50];
static int receivedRows = 0;

/**
 * @brief Function to process the received message (in pc_terminal).
 * @param msgType Message type enum
 * @param uint8_t* Pointer to data to process
 * @return '.' if flight is finished, else 1
 * @author Kristóf
 */
int processMsg(msgType type, uint8_t *pData)
{
	int ret = 1;
	switch (type)
	{
	case TELEM:
	{
		// Print telemetry to terminal
		printf("Mode: %d | ", pData[0]);
		printf("Motors: %3d %3d %3d %3d | ", to_i16(&pData[1]), to_i16(&pData[3]), to_i16(&pData[5]), to_i16(&pData[7]));
		printf("Angles: %6d %6d %6d | ", to_i16(&pData[9]), to_i16(&pData[11]), to_i16(&pData[13]));
		printf("Rates: %6d %6d %6d | ", to_i16(&pData[15]), to_i16(&pData[17]), to_i16(&pData[19]));
		printf("Bat: %4d | Temp: %4d | Pressure: %6d | ", to_ui16(&pData[21]), to_i32(&pData[23]), to_i32(&pData[27]));
		printf("P: %4d | P1: %4d | P2: %4d | HEI: %4d\n", to_i16(&pData[31]), to_i16(&pData[33]), to_i16(&pData[35]), to_i16(&pData[37]));
		
		// Send to processing (pitch, roll, yaw angles)
		if (tcp_enabled && (client_fd != -1) && (sock != -1))
		{
			char TCPmsg[20];
			snprintf(TCPmsg, sizeof(TCPmsg), "%d %d %d\n", to_i16(&pData[9]), to_i16(&pData[11]), to_i16(&pData[13]));
			if(send(sock, TCPmsg, strlen(TCPmsg), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
			{
				printf("Can't send to socket, closing it\n");
				tcp_enabled = false;
				closeSocket();
			} 
		}
		
		break;
	}

	case LOG:
	{
		// Create logfile with the timestamp of the first row in its name -> unique log file every time 
		if (!logFileExist)
		{
			printf("No log file yet, create new one\n");
			strcpy(logName, "log_");

			// Append current time
			struct timeval tv;
			time_t nowtime;
			struct tm *nowtm;
			char tmbuf[40];

			gettimeofday(&tv, NULL);
			nowtime = tv.tv_sec;
			nowtm = localtime(&nowtime);
			strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
			strncat(logName, tmbuf, sizeof(tmbuf));
			strncat(logName, ".txt", (sizeof(logName) - strlen(logName))); // Protected against overflow
			logFileExist = true;
			printf("Log file created with name: %s\n", logName);
		}
		
		// Open log file and write one row in it
		FILE *fp = fopen(logName, "a");
		fprintf(fp, "%10d | ", to_ui32(&pData[0]));
		if (pData[4] == Telemetry)
		{
			// TELEMETRY
			fprintf(fp, "Mode: %d | ", pData[5]);
			fprintf(fp, "Motors: %3d %3d %3d %3d | ", to_i16(&pData[6]), to_i16(&pData[8]), to_i16(&pData[10]), to_i16(&pData[12]));
			fprintf(fp, "Angles: %6d %6d %6d | ", to_i16(&pData[14]), to_i16(&pData[16]), to_i16(&pData[18]));
			fprintf(fp, "Rates: %6d %6d %6d | ", to_i16(&pData[20]), to_i16(&pData[22]), to_i16(&pData[24]));
			fprintf(fp, "Bat: %4d | Temp: %4d | Pressure: %6d | ", to_ui16(&pData[26]), to_i32(&pData[28]), to_i32(&pData[32]));
			fprintf(fp, "P: %4d | P1: %4d | P2: %4d | HEI: %4d\n", to_i16(&pData[36]), to_i16(&pData[38]), to_i16(&pData[40]), to_i16(&pData[42]));
		}
		else if (pData[4] == ModeChg)
		{
			// MODE CHG
			if (pData[7])
			{
				fprintf(fp, "Mode change request from %d to %d: OK\n", pData[5], pData[6]);
			}
			else
			{
				fprintf(fp, "Mode change request from %d to %d: NOT CHANGED\n", pData[5], pData[6]);
			}
		}
		else if (pData[4] == Command)
		{
			// CMD
			fprintf(fp, "A: %u, Z: %u, LEFT: %u, RIGHT: %u, UP: %u, DOWN: %u, Q: %u, W: %u, ", ((pData[5] >> 7) & 0x01), ((pData[5] >> 6) & 0x01), ((pData[5] >> 5) & 0x01), ((pData[5] >> 4) & 0x01), ((pData[5] >> 3) & 0x01), ((pData[5] >> 2) & 0x01), ((pData[5] >> 1) & 0x01), (pData[5] & 0x01));
			fprintf(fp, "ESC: %u, FIRE: %u, U: %u, J: %u, I: %u, K: %u, O: %u, L: %u, ", ((pData[6] >> 7) & 0x01), ((pData[6] >> 6) & 0x01), ((pData[6] >> 5) & 0x01), ((pData[6] >> 4) & 0x01), ((pData[6] >> 3) & 0x01), ((pData[6] >> 2) & 0x01), ((pData[6] >> 1) & 0x01), (pData[6] & 0x01));
			fprintf(fp, "ROLL: %u, PITCH: %u, YAW: %u, THROTTLE: %u\n", pData[7], pData[8], pData[9], pData[10]);
		}
		else if (pData[4] == Profiling)
		{
			// PROFILING
			fprintf(fp, "Control loop: %10d, Timer flag: %10d, Yaw mode: %10d, Full mode: %10d, Raw mode: %10d, Height mode: %10d, Logging time: %10d, SQRT time: %10d\n", 
			to_ui32(&pData[5]), to_ui32(&pData[9]), to_ui32(&pData[13]), to_ui32(&pData[17]), to_ui32(&pData[21]), to_ui32(&pData[25]), to_ui32(&pData[29]), to_ui32(&pData[33]));
		}
		else if (pData[4] == Full)
		{
			// FULL
			fprintf(fp, "Flash is full\n");
		}
		else
		{
			// ERROR
			fprintf(fp, "Invalid log type\n");
		}
		fclose(fp);

		// Print number of received rows to terminal
		receivedRows++;
		printf("\r%d rows received... ", receivedRows);
		break;
	}

	case ACK:
	{
		printf("ACK arrived: %d\n", pData[0]);
		// Mode 8
		if (pData[0] == 8)
		{
			if(useBluetooth == false) printf("Changing to bluetooth!\n");
			useBluetooth = true;
			serial_port_close();
		}
		// Config arrived
		else if (pData[0] == 'C')
		{
			printf("Config received by drone\n");
		}
		// Flight finished
		else if (pData[0] == '.')
		{
			printf("Drone finished flying\n");
			ret = pData[0];
		}		
		// Every other mode
		else
		{
			if (useBluetooth == true) printf("Changing to uart!\n");
			ble_port_close();
			useBluetooth = false;
		}
		
		break;
	}
	
	default:
	{
		printf("Type error at processing messages!\n");
		break;
	}
	}

	return ret;
}

/**
 * @brief Function to process the received message (in pc_gui).
 * @param msgType Message type enum
 * @param uint8_t* Pointer to data to process
 * @param pointers Structure of pointers which points to data fields used by GUI
 * @return '.' if flight is finished, else returns 1
 * @author Kristóf
 */
int processMsgGui(msgType type, uint8_t *pData, pointers pointers)
{
	int ret = 1;

	char msg[100];

	switch (type)
	{
	case TELEM:
	{
		// Print telemetry to terminal in debug mode
		if (DEBUG_MODE)
		{
			printf("Mode: %d | ", pData[0]);
			printf("Motors: %3d %3d %3d %3d | ", to_i16(&pData[1]), to_i16(&pData[3]), to_i16(&pData[5]), to_i16(&pData[7]));
			printf("Angles: %6d %6d %6d | ", to_i16(&pData[9]), to_i16(&pData[11]), to_i16(&pData[13]));
			printf("Rates: %6d %6d %6d | ", to_i16(&pData[15]), to_i16(&pData[17]), to_i16(&pData[19]));
			printf("Bat: %4d | Temp: %4d | Pressure: %6d | ", to_ui16(&pData[21]), to_i32(&pData[23]), to_i32(&pData[27]));
			printf("P: %4d | P1: %4d | P2: %4d | HEI: %4d\n", to_i16(&pData[31]), to_i16(&pData[33]), to_i16(&pData[35]), to_i16(&pData[37]));
		}

		// Send to processing (pitch, roll, yaw angles)
		if (tcp_enabled && (client_fd != -1) && (sock != -1))
		{
			char TCPmsg[20];
			snprintf(TCPmsg, sizeof(TCPmsg), "%d %d %d\n", to_i16(&pData[9]), to_i16(&pData[11]), to_i16(&pData[13]));
			if(send(sock, TCPmsg, strlen(TCPmsg), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
			{
				printf("Can't send to socket, closing it\n");
				tcp_enabled = false;
				closeSocket();
			} 
		}

		// Send telemetry data to GUI
		pointers.motorValues[0] = to_i16(&pData[1]) / 600.0f * 100.0f;
		pointers.motorValues[1] = to_i16(&pData[3]) / 600.0f * 100.0f;
		pointers.motorValues[2] = to_i16(&pData[5]) / 600.0f * 100.0f;
		pointers.motorValues[3] = to_i16(&pData[7]) / 600.0f * 100.0f;

		pointers.gains[0] = to_i16(&pData[31]);
		pointers.gains[1] = to_i16(&pData[33]);
		pointers.gains[2] = to_i16(&pData[35]);
		pointers.gains[3] = to_i16(&pData[37]);

		break;
	}

	case LOG:
	{
		// Create logfile with the timestamp of the first row in its name -> unique log file every time 
		if (!logFileExist)
		{
			printf("No log file yet, create new one\n");
			strcpy(logName, "log_");

			// Append current time
			struct timeval tv;
			time_t nowtime;
			struct tm *nowtm;
			char tmbuf[40];

			gettimeofday(&tv, NULL);
			nowtime = tv.tv_sec;
			nowtm = localtime(&nowtime);
			strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
			strncat(logName, tmbuf, sizeof(tmbuf));
			strncat(logName, ".txt", (sizeof(logName) - strlen(logName))); // Protected against overflow
			logFileExist = true;
			printf("Log file created with name: %s\n", logName);
		}
		
		// Open log file and write one row
		FILE *fp = fopen(logName, "a");
		fprintf(fp, "%10d | ", to_ui32(&pData[0]));
		if (pData[4] == Telemetry)
		{
			// TELEMETRY
			fprintf(fp, "Mode: %d | ", pData[5]);
			fprintf(fp, "Motors: %3d %3d %3d %3d | ", to_i16(&pData[6]), to_i16(&pData[8]), to_i16(&pData[10]), to_i16(&pData[12]));
			fprintf(fp, "Angles: %6d %6d %6d | ", to_i16(&pData[14]), to_i16(&pData[16]), to_i16(&pData[18]));
			fprintf(fp, "Rates: %6d %6d %6d | ", to_i16(&pData[20]), to_i16(&pData[22]), to_i16(&pData[24]));
			fprintf(fp, "Bat: %4d | Temp: %4d | Pressure: %6d | ", to_ui16(&pData[26]), to_i32(&pData[28]), to_i32(&pData[32]));
			fprintf(fp, "P: %4d | P1: %4d | P2: %4d | HEI: %4d\n", to_i16(&pData[36]), to_i16(&pData[38]), to_i16(&pData[40]), to_i16(&pData[42]));
		}
		else if (pData[4] == ModeChg)
		{
			// MODE CHG
			if (pData[7])
			{
				fprintf(fp, "Mode change request from %d to %d: OK\n", pData[5], pData[6]);
			}
			else
			{
				fprintf(fp, "Mode change request from %d to %d: NOT CHANGED\n", pData[5], pData[6]);
			}
		}
		else if (pData[4] == Command)
		{
			// CMD
			fprintf(fp, "A: %u, Z: %u, LEFT: %u, RIGHT: %u, UP: %u, DOWN: %u, Q: %u, W: %u, ", ((pData[5] >> 7) & 0x01), ((pData[5] >> 6) & 0x01), ((pData[5] >> 5) & 0x01), ((pData[5] >> 4) & 0x01), ((pData[5] >> 3) & 0x01), ((pData[5] >> 2) & 0x01), ((pData[5] >> 1) & 0x01), (pData[5] & 0x01));
			fprintf(fp, "ESC: %u, FIRE: %u, U: %u, J: %u, I: %u, K: %u, O: %u, L: %u, ", ((pData[6] >> 7) & 0x01), ((pData[6] >> 6) & 0x01), ((pData[6] >> 5) & 0x01), ((pData[6] >> 4) & 0x01), ((pData[6] >> 3) & 0x01), ((pData[6] >> 2) & 0x01), ((pData[6] >> 1) & 0x01), (pData[6] & 0x01));
			fprintf(fp, "ROLL: %u, PITCH: %u, YAW: %u, THROTTLE: %u\n", pData[7], pData[8], pData[9], pData[10]);
		}
		else if (pData[4] == Profiling)
		{
			// PROFILING
			fprintf(fp, "Control loop: %10d, Timer flag: %10d, Yaw mode: %10d, Full mode: %10d, Raw mode: %10d, Height mode: %10d, Logging time: %10d, SQRT time: %10d\n", 
			to_ui32(&pData[5]), to_ui32(&pData[9]), to_ui32(&pData[13]), to_ui32(&pData[17]), to_ui32(&pData[21]), to_ui32(&pData[25]), to_ui32(&pData[29]), to_ui32(&pData[33]));
		}
		else if (pData[4] == Full)
		{
			// FULL
			fprintf(fp, "Flash is full\n");
		}
		else
		{
			// ERROR
			fprintf(fp, "Invalid log type\n");
		}
		fclose(fp);

		// Print number of received rows to terminal
		receivedRows++;
		printf("\r%d rows received... ", receivedRows);
		break;
	}

	case ACK:
	{
		printf("ACK arrived: %d\n", pData[0]);
		// Mode 8
		if (pData[0] == 8)
		{
			*(pointers.ackMode) = pData[0];
			if(useBluetooth == false) printf("Changing to bluetooth!\n");
			useBluetooth = true;
			serial_port_close();
		}
		// Config arrived
		else if (pData[0] == 'C')
		{
			printf("Config received by drone\n");
		}
		// Flight finished
		else if (pData[0] == '.')
		{
			printf("Drone finished flying\n");
			ret = pData[0];
		}
		// Every other mode
		else
		{
			*(pointers.ackMode) = pData[0];
			if (useBluetooth == true) printf("Changing to uart!\n");
			ble_port_close();
			useBluetooth = false;
		}
		break;
	}
	
	default:
	{
		printf("Type error at processing messages!\n");
		strncat(pointers.text, "Type error at processing messages!\n", (TEXT_LEN - strlen(pointers.text) - 1)); // Protected against overflow (I hope so)
		break;
	}
	}

	return ret;
}

/**
 * @brief Function to pack and send the messages based on their type.
 * Both pc_terminal and pc_gui uses this function to send.
 * @param msgType Message type enum
 * @param uint8_t* Pointer to data to send
 * @author Kristóf
 */
void packMessage(msgType type, uint8_t *pData)
{
	// We have a 8 bit XOR check sum field at the end of the message
	uint8_t checkSum = 0;

	// Start character
	serial_port_putchar('?');
	checkSum ^= '?';

	// Message type
	serial_port_putchar(type);
	checkSum ^= type;

	// Add the length and the data to the message
	switch (type)
	{
	case CFG:
	{
		serial_port_putchar(CFG_SIZE);
		checkSum ^= CFG_SIZE;
		for (uint8_t i = 0; i < CFG_SIZE; i++)
		{
			// Control params
			serial_port_putchar(pData[i]);
			checkSum ^= pData[i];
		}
		break;
	}
		
	case MODE:
	{
		serial_port_putchar(1);
		checkSum ^= 1;
		serial_port_putchar(pData[0]);
		checkSum ^= pData[0];
		break;
	}
		
	case CMD:
	{
		serial_port_putchar(CMD_SIZE);
		checkSum ^= CMD_SIZE;
		for (int i = 0; i < CMD_SIZE; i++)
		{
			serial_port_putchar(pData[i]);
			checkSum ^= pData[i];
		}
		break;
	}

	default:
	{
		serial_port_putchar(1);	// Length 1
		checkSum ^= 1;
		serial_port_putchar(0);	// Emergeny stop data = 0
		checkSum ^= 0;
		break;
	}
	}

	// Last field of message is the checksum
	serial_port_putchar(checkSum);
}

/**
 * @brief Function to process incoming characters (pc_terminal).
 * Calls processMsg when a message is finished.
 * @param uint8_t Received character
 * @param recMachine Pointer to receiving state machine
 * @return ret_val of processMsg() if it was called, else 0 
 * @author Kristóf
 */
int unpackMessage(uint8_t c, recMachine *SM)
{	
	uint8_t error = 0;
	int ret = 0;
	// State machine to process incoming message byte-to-byte
  switch (SM->actualState)
	{
	case START:
	{
		// Step next state if start character received
		if (c == '?')
		{
			SM->recCsum = 0;
			SM->idx = 0;
			SM->actualState = TYPE;
		}
		break;
	}

	case TYPE:
	{
		SM->recType = (msgType)c;
		if (SM->recType != TELEM && SM->recType != LOG && SM->recType != ACK && SM->recType != DEBUG)
		{
			printf("Message type error at receiving!\n");
			error = 1; // Start again as we have an error
		}

		// Step either debug state or length state
		if (SM->recType == DEBUG)
		{
			SM->actualState = SDEBUG;
		}
		else
		{
			SM->actualState = LENGTH;
		}
		break;
	}

	case LENGTH:
	{
		SM->recLength = c;
		SM->actualState = DATA;
		break;
	}

	case DATA:
	{
		// Read data bits (length times)
		SM->msg[SM->idx++] = c;
		SM->recLength--;
		if (!(SM->recLength))
			SM->actualState = CSUM;
		break;
	}

	case CSUM:
	{
		if (c != SM->recCsum)
		{
			printf("Checksum error at receiving!\n");
		}
		// Process message if check sum was correct
		else
		{
			ret = processMsg(SM->recType, SM->msg);
		}
		SM->actualState = START;
		break;
	}

	case SDEBUG:
	{
		// Print debug message to terminal
		printf("%c", c);
		if (c == '\n')
		{
			SM->actualState = START;
		}
		break;
	}
	
	default:
	{
		// Reset state machine
		printf("Error at receiving!\n");
		SM->actualState = START;
		break;
	}
	}

	// Add received character to checksum
	SM->recCsum ^= c;

	// Restart state machine if there was an error
	if (error)
	{
		SM->actualState = START;
	}

	return ret;
}

/**
 * @brief Function to process incoming characters (pc_gui).
 * Calls processMsgGui when a message is finished.
 * @param uint8_t Received character
 * @param recMachine* Pointer to receiving state machine
 * @param pointers Structure of pointers which points to data fields used by GUI
 * @return ret_val of processMsgGui() if it was called, else 0 
 * @author Kristóf
 */
int unpackMessageGui(uint8_t c, recMachine *SM, pointers pointers)
{
	uint8_t error = 0;
	int ret = 0;
	// State machine to process incoming message byte-to-byte
    switch (SM->actualState)
	{
	case START:
	{
		// Step next state if start character received
		if (c == '?')
		{
			SM->recCsum = 0;
			SM->idx = 0;
			SM->actualState = TYPE;
		}
		break;
	}

	case TYPE:
	{
		SM->recType = (msgType)c;
		if (SM->recType != TELEM && SM->recType != LOG && SM->recType != ACK && SM->recType != DEBUG)
		{
			printf("Message type error at receiving!\n");
			// Print to GUI text window
			strncat(pointers.text, "Message type error at receiving!\n", (TEXT_LEN - strlen(pointers.text) - 1)); // Protected against overflow (I hope so)
			error = 1; // Start again as we have an error
		}

		// Step either debug state or length state
		if (SM->recType == DEBUG)
		{
			SM->actualState = SDEBUG;
		}
		else
		{
			SM->actualState = LENGTH;
		}
		break;
	}

	case LENGTH:
	{
		SM->recLength = c;
		SM->actualState = DATA;
		break;
	}

	case DATA:
	{
		// Read data bits (length times)
		SM->msg[SM->idx++] = c;
		SM->recLength--;
		if (!(SM->recLength))
			SM->actualState = CSUM;
		break;
	}

	case CSUM:
	{
		if (c != SM->recCsum)
		{
			printf("Checksum error at receiving!\n");
			// Print to GUI text window
			strncat(pointers.text, "Checksum error at receiving!\n", (TEXT_LEN - strlen(pointers.text) - 1)); // Protected against overflow (I hope so)
		}
		// Process message if check sum was correct
		else
		{
			ret = processMsgGui(SM->recType, SM->msg, pointers);
		}
		SM->actualState = START;
		break;
	}

	case SDEBUG:
	{
		// Print debug message to terminal and GUI text window
		char msg[10];
		printf("%c", c);
		snprintf(msg, sizeof(msg), "%c", c);
		strncat(pointers.text, msg, (TEXT_LEN - strlen(pointers.text) - 1)); // Protected against overflow (I hope so)
		if (c == '\n')
		{
			SM->actualState = START;
		}
		break;
	}
	
	default:
	{
		// Reset state machine
		printf("Error at receiving!\n");
		// Print to GUI text window
		strncat(pointers.text, "Error at receiving!\n", (TEXT_LEN - strlen(pointers.text) - 1)); // Protected against overflow (I hope so)
		SM->actualState = START;
		break;
	}
	}

	// Add received character to checksum
	SM->recCsum ^= c;

	// Restart state machine if there was an error
	if (error)
	{
		SM->actualState = START;
	}

	return ret;
}

/**
 * @brief Function to process keyboard inputs. Both pc_terminal and
 * pc_gui use this function.
 * @param char Input character from keyboard
 * @param uint8_t* Pointer to pilotCmd array
 * @return Requested mode in case of mode change, else -1 
 * @author Kristóf
 */
int8_t processKeyboard(char c, uint8_t *cmd)
{
	int8_t ret = -1;
	uint8_t mode;

	// For arrow and escape keys
  // 27 is start escape sequence
	if(c == 27) {

		char c2 = term_getchar_nb(); 

		if(c2 == -1) { //Esc key is mode change
			mode = 1;
			ret = 1;
			packMessage(MODE, &mode);
		}
		else if(c2 == 91) {
			char c3 = term_getchar_nb();

			switch(c3) {
				case /*left*/ 68:
				{
					cmd[0] |= (0x01 << 5);
					break;
				}
				case /*right*/ 67:
				{
					cmd[0] |= (0x01 << 4);
					break;
				}
				case /*up*/ 65:
				{
					cmd[0] |= (0x01 << 3);
					break;
				}
				case /*down*/ 66:
				{
					cmd[0] |= (0x01 << 2);
					break;
				}
			}
		}
	}

	switch (c)
	{	
	// Usual keys
	case 'a':
	{
		cmd[0] |= (0x01 << 7);
		break;
	}

	case 'z':
	{
		cmd[0] |= (0x01 << 6);
		break;
	}

	case 'q':
	{
		cmd[0] |= (0x01 << 1);
		break;
	}

	case 'w':
	{
		cmd[0] |= 0x01;
		break;
	}
	case 'u':
	{
		cmd[1] |= (0x01 << 5);
		break;
	}

	case 'j':
		cmd[1] |= (0x01 << 4);
		break;
	
	case 'i':
		cmd[1] |= (0x01 << 3);
		break;
	
	case 'k':
		cmd[1] |= (0x01 << 2);
		break;
	
	case 'o':
		cmd[1] |= (0x01 << 1);
		break;
	
	case 'l':
		cmd[1] |= 0x01;
		break;

	case '.':
		cmd[6] |= (0x01 << 7);
		break;

	case 'y':
		cmd[6] |= (0x01 << 1);
		break;

	case 'h':
		cmd[6] |= 0x01;
		break;

	// Mode change keys (send mode change request immediately)
	case '0':
		mode = SafeMode; 
		ret = SafeMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
	
	case '1':
		mode = PanicMode; 
		ret = PanicMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
	
	case '2':
		mode = ManualMode; 
		ret = ManualMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
	
	case '3':
		mode = CalibrationMode; 
		ret = CalibrationMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
		
	case '4':
		mode = YawControlledMode; 
		ret = YawControlledMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
		
	case '5':
		mode = FullControllMode; 
		ret = FullControllMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
		
	case '6':
		mode = RawMode; 
		ret = RawMode;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;
		
	case '7':
		mode = HeightControl; 
		ret = HeightControl;
		if (fd_serial_port == -1) serial_port_open(SERIAL_PORT);
		packMessage(MODE, &mode);
		break;

	case '8':
	{
		// Open bluetooth port
		ble_port_open(BT_PORT);

		// Change to wireless mode if bluetooth opened successfully
		if (fd_ble_port != -1)
		{
			mode = WirelessControl; 
			ret = WirelessControl;
			packMessage(MODE, &mode);
		}
		else
		{
			printf("Couldn't change to wireless mode. No bluetooth connection.\n");
		}
		break;
	}

	default:
		break;
	}

	return ret;
}
#include "joy.h"

#define NAME_LENGTH 128
#define JS_DEV	"/dev/input/js0"

// Variables to store joystick related values
int fd;
unsigned char axes = 2;
unsigned char buttons = 2;
int *axis;
char *button;

/**
 * @brief Function to open joystick communication.
 * Similar to joystick example's code.
 * @return 0 if joystick opened successful, -1 on error
 */
int openJoy(void)
{

	int version = 0x000800;
	char name[NAME_LENGTH] = "Unknown";

	if ((fd = open(JS_DEV, O_RDONLY)) < 0) {
		perror("jstest");
		return -1;
	}

	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &buttons);
	ioctl(fd, JSIOCGNAME(NAME_LENGTH), name);

	printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
		name, axes, buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);

    fcntl(fd, F_SETFL, O_NONBLOCK);

    axis = (int*)calloc(axes, sizeof(int));
    button = (char*)calloc(buttons, sizeof(char));

    return 0;
}

/**
 * @brief Function to readout joystick axes and buttons.
 * Based on joystick example's Event interface, single line readout code.
 * @param uint8_t* Pointer to pilotCmd array
 * @author Kristóf
 */
void getJoyValues(uint8_t* pilotCmd)
{
    int i;
    struct js_event js;
    
    // Read while there are new events
    while (read(fd, &js, sizeof(struct js_event)) == sizeof(struct js_event))  
    {
        switch(js.type & ~JS_EVENT_INIT)
        {
        case JS_EVENT_BUTTON:
            button[js.number] = js.value;
            break;
        case JS_EVENT_AXIS:
            axis[js.number] = js.value;
            break;
        }
    }

    if (errno != EAGAIN) 
    {
        perror("\njstest: error reading");
    }

    // Handle axes
    if (axes) 
    {
        // Map between 0 and 255
        uint8_t roll   = (axis[0] + 32767) * 255 / 65534;
        uint8_t pitch  = (((-1)*axis[1]) + 32767) * 255 / 65534;
        uint8_t yaw    = (axis[2] + 32767) * 255 / 65534;
        uint8_t lift   = (((-1)*axis[3]) + 32767) * 255 / 65534;

        // Put axis values into pilotCmd array
        pilotCmd[2] = roll;    // Roll
        pilotCmd[3] = pitch;   // Pitch
        pilotCmd[4] = yaw;     // Yaw
        pilotCmd[5] = lift;    // Lift
    }

    // Handle buttons
    if (buttons) 
    {
        // Fire button -> Abort
        if (button[0])
        {
            pilotCmd[1] |= (0x01 << 6);
        }
    }
}
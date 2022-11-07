// C++ includes
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

// ImGui includes
#include "../imgui-master/imgui.h"
#include "../imgui-master/backends/imgui_impl_sdl.h"
#include "../imgui-master/backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

// My includes
#include "../communication/protocol.h"
#include "../communication/joy.h"
#include "../communication/config.h"

// C includes
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

// Struct to pass values to DrawGUI function
typedef struct 
{
    int* p;
    int* p1;
    int* p2;
    int* hei;
    uint8_t* reqMode;
    uint8_t* ackMode;

    float* motorValues;
    float* joy;
    char* text;
} pGuiValues;

// Struct to pass values from main thread to GUI thread
typedef struct
{
    float joy[4] = {-1000.0f, -1000.0f, -1000.0f, -1000.0f};
    float motorValues[4] = {-1000.0f, -1000.0f, -1000.0f, -1000.0f};
    int8_t reqMode = -1;
    int8_t ackMode = -1;
    char text[TEXT_LEN] = "";
    int16_t p = -1000;
    int16_t p1 = -1000;
    int16_t p2 = -1000;
    int16_t hei = -1000;
} toGUI;

// Struct to pass values from the GUI thread to the main thread
typedef struct
{
    int8_t reqMode = -1;
    int8_t chgP = 0;
    int8_t chgP1 = 0;
    int8_t chgP2 = 0;
    int8_t chgHei = 0;
} toTerm;

// Mutexes and queues for message passing between threads
std::mutex syncToGui;
std::mutex syncToTerm;
std::queue<toGUI> qToGUI;
std::queue<toTerm> qToTerm;

/**
 * @brief Cleans up everything related to GUI.
 * Called once at the end of program.
 * @param SDL_Window* SDL window pointer
 * @param SDL_GLContext SDL OpenGL context
 * @author Kristóf
 */
void cleanUpGUI(SDL_Window* window, SDL_GLContext gl_context)
{
    // Cleanup ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/**
 * @brief Draws everything in the GUI window. Runs every frame, in separate thread
 * @param pGuiValues Struct of pointers which point to the values used by the GUI
 * @param SDL_Window* SDL window pointer
 * @param IMGuiIO& Reference to the main configuration and I/O between the application and ImGui
 * @author Kristóf
 */
void drawGUI(pGuiValues values, SDL_Window* window, ImGuiIO& io)
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Color of clean background
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Struct to terminal queue
    toTerm dataToSendTerm;

    // Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        ImGui::Begin("Drone GUI"); // Create a window called "Drone GUI" and append into it.
        
        // Show frame rate
        ImGui::Text("Frame rate: Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        // Mode selection section -> active mode is green
        ImGui::Text("Mode buttons");
        for (int i = 0; i < 9; i++)
        {
            char buttonText[20];
            // Set the active mode's color to green
            if (*(values.ackMode) == i)
            {
                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
            }
            // Write name on mode buttons
            switch (i)
            {
                case SafeMode:
                {
                    strcpy(buttonText, "Safe");
                    break;
                }
                case PanicMode:
                {
                    strcpy(buttonText, "Panic");
                    break;
                }
                case ManualMode:
                {
                    strcpy(buttonText, "Manual");
                    break;
                }
                case CalibrationMode:
                {
                    strcpy(buttonText, "Calibration");
                    break;
                }
                case YawControlledMode:
                {
                    strcpy(buttonText, "Yaw-control");
                    break;
                }
                case FullControllMode:
                {
                    strcpy(buttonText, "Full-control");
                    break;
                }
                case RawMode:
                {
                    strcpy(buttonText, "Raw");
                    break;
                }
                case HeightControl:
                {
                    strcpy(buttonText, "Height-control");
                    break;
                }
                case WirelessControl:
                {
                    strcpy(buttonText, "Wireless");
                    break;
                }
                default:
                    break;
            }

            // Change the mode if button was clicked
            if (ImGui::Button(buttonText))
            { 
                // Change the requested mode
                dataToSendTerm.reqMode = i;
            }

            // Change color back to normal
            if (*(values.ackMode) == i)
            {
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
            
            // Put the buttons in the same line
            if (i < 8) ImGui::SameLine();
        }

        // Text message about the requested and the active mode
        ImGui::Text("Requested mode = %u", *(values.reqMode));
        ImGui::SameLine();
        ImGui::Text("Current mode = %u", *(values.ackMode));
        
        // Gain configuration section
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;

        // Yaw P value
        if ((*(values.ackMode) < YawControlledMode) || (*(values.ackMode) > RawMode)) ImGui::BeginDisabled(true);
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left1", ImGuiDir_Left)) { dataToSendTerm.chgP = 1; }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right1", ImGuiDir_Right)) { dataToSendTerm.chgP = -1; }
        ImGui::PopButtonRepeat();
        if ((*(values.ackMode) < YawControlledMode) || (*(values.ackMode) > RawMode)) ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(true);
        ImGui::SliderInt("Yaw control P", values.p, 0, 100);   // Edit 1 int using a slider from 0 to 100
        ImGui::EndDisabled();

        // Pitch/roll P1 value
        if ((*(values.ackMode) < FullControllMode) || (*(values.ackMode) > RawMode)) ImGui::BeginDisabled(true);
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left2", ImGuiDir_Left)) { dataToSendTerm.chgP1 = -1; }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right2", ImGuiDir_Right)) { dataToSendTerm.chgP1 = 1; }
        ImGui::PopButtonRepeat();
        if ((*(values.ackMode) < FullControllMode) || (*(values.ackMode) > RawMode)) ImGui::EndDisabled();

        ImGui::SameLine();
        
        ImGui::BeginDisabled(true);
        ImGui::SliderInt("Roll/pitch control P1", values.p1, 0, 100);
        ImGui::EndDisabled();

        // Pitch/roll P2 value
        if ((*(values.ackMode) < FullControllMode) || (*(values.ackMode) > RawMode)) ImGui::BeginDisabled(true);
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left3", ImGuiDir_Left)) { dataToSendTerm.chgP2 = 1; }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right3", ImGuiDir_Right)) { dataToSendTerm.chgP2 = -1; }
        ImGui::PopButtonRepeat();
        if ((*(values.ackMode) < FullControllMode) || (*(values.ackMode) > RawMode)) ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(true);
        ImGui::SliderInt("Roll/pitch control P2", values.p2, 0, 100);
        ImGui::EndDisabled();

        // Height control P value
        if (*(values.ackMode) != HeightControl) ImGui::BeginDisabled(true);
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left4", ImGuiDir_Left)) { dataToSendTerm.chgHei = -1; }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right4", ImGuiDir_Right)) { dataToSendTerm.chgHei = 1; }
        ImGui::PopButtonRepeat();
        if (*(values.ackMode) != HeightControl) ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(true);
        ImGui::SliderInt("Height control P", values.hei, 0, 100);
        ImGui::EndDisabled();

        // Joystick sliders
        ImGui::Text("Joy controls");

        ImGui::BeginDisabled(true);
        ImGui::PushID("Joy");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(200, 4));
        ImGui::VSliderFloat("Pitch", ImVec2(40, 160), &values.joy[0], -100.0f, 100.0f); // Pitch
        ImGui::SameLine(); 
        ImGui::VSliderFloat("Throttle", ImVec2(40, 160), &values.joy[3], 0.0f, 100.0f, "%.1f%%"); // Throttle
        ImGui::PopStyleVar();
        ImGui::SliderFloat2("Roll & Yaw", &values.joy[1], -100.0f, 100.0f); // It gets joy[1] and joy[2]
        ImGui::PopID();
        ImGui::EndDisabled();
        
        // Motor values
        ImGui::Text("Motor values");

        ImGui::BeginDisabled(true);
        ImGui::PushID("Motors");
        const ImVec2 small_slider_size(40, 80);
        ImGui::VSliderFloat("##v", small_slider_size, &values.motorValues[0], 0.0f, 150.0f, "%.1f%%");
        ImGui::SameLine();
        ImGui::VSliderFloat("##v", small_slider_size, &values.motorValues[1], 0.0f, 150.0f, "%.1f%%");
        ImGui::SameLine();
        ImGui::VSliderFloat("##v", small_slider_size, &values.motorValues[2], 0.0f, 150.0f, "%.1f%%");
        ImGui::SameLine();
        ImGui::VSliderFloat("##v", small_slider_size, &values.motorValues[3], 0.0f, 150.0f, "%.1f%%");
        ImGui::PopID();
        ImGui::EndDisabled();

        ImGui::End();
    }

    // Show the text output
    {
        ImGui::Begin("Text output");

        ImGui::BeginDisabled(true);
        static ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly;
        ImGui::InputTextMultiline("##source", values.text, IM_ARRAYSIZE(values.text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);
        ImGui::EndDisabled();

        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    // Put items into queue
    {
        // Lock with lock_guard
        std::lock_guard<std::mutex> myLock(syncToTerm);

        qToTerm.push(dataToSendTerm);

        // Unlocks as lock_guard goes out of scope
    }

    return;
}

/**
 * @brief Thread function for GUI. Runs completely separated from
 * the PC terminal. Gets and sends the values in mutex protected queues.
 * @author Kristóf
 */
void guiThread()
{
    printf("GUI init\n");
    
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    // Decide GL+GLSL versions
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Drone GUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.WantCaptureKeyboard = false;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize GUI variables
    static int p = P_VALUE;
    static int p1 = P1_VALUE;
    static int p2 = P2_VALUE;
    static int hei = HEIGHT_GAIN;
    static uint8_t reqMode = SafeMode;
    static uint8_t ackMode = SafeMode;

    static float motorValues[4] = { 0.00f, 0.00f, 0.00f, 0.00f };

    // Pitch, roll, yaw, throttle (in that order)
    static float joy[4] = {0, 0, 0, 0};

    static char text[TEXT_LEN] = "Messages output:\n";
    
    // Struct to pass values to draw function (also fill the struct)
    pGuiValues guiValues;
    guiValues.reqMode = &reqMode;
    guiValues.ackMode = &ackMode;
    guiValues.p = &p; guiValues.p1 = &p1; guiValues.p2 = &p2; guiValues.hei = &hei;
    guiValues.motorValues = motorValues;
    guiValues.joy = joy;
    guiValues.text = text;

    printf("GUI started\n");

    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Get items from queue
        {
            // Lock with lock_guard
            std::lock_guard<std::mutex> myLock(syncToGui);

            // Save all values from the queue
            while (!qToGUI.empty())
            {
                toGUI rec = qToGUI.front();
                if (rec.reqMode != -1) reqMode = (uint8_t)rec.reqMode;
                if (rec.ackMode != -1) ackMode = (uint8_t)rec.ackMode;
                for (uint8_t i = 0; i < 4; i++)
                {
                    if (rec.motorValues[i] != -1000.0f) motorValues[i] = rec.motorValues[i];
                    if (rec.joy[i] != -1000.0f) joy[i] = rec.joy[i];
                }
                if (strlen(rec.text) > 0) strncpy(text, rec.text, TEXT_LEN);
                if (rec.p != -1000) p = (int)rec.p;
                if (rec.p1 != -1000) p1 = (int)rec.p1;
                if (rec.p2 != -1000) p2 = (int)rec.p2;
                if (rec.hei != -1000) hei = (int)rec.hei;
                qToGUI.pop();
            }
            // Unlocks as lock_guard goes out of scope
        }

        // Draw the windows
        drawGUI(guiValues, window, io);
    }

    // Clean up GUI
    cleanUpGUI(window, gl_context);

    printf("GUI closed\n");
}

/*----------------------------------------------------------------
 * main -- execute terminal
 *----------------------------------------------------------------
 */

/**
 * @brief The main -> completely based on pc_terminal.c
 * Handles PC-drone communication, processes keyboard inputs.
 * @author Kristóf
 */
int main(int, char**) 
{
    // Init console
    term_initio();
	term_puts("\nTerminal program - Embedded Real-Time Systems\n");

    // GUI thread
    std::thread gui;

    // Init section like in pc_terminal.c
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

    // Initialize GUI variables
    static int16_t gains[4] = {P_VALUE, P1_VALUE, P2_VALUE, HEIGHT_GAIN};
    static uint8_t ackMode = 0;
    static float motorValues[4] = { 0.00f, 0.00f, 0.00f, 0.00f };
    static float joy[4] = {0, 0, 0, 0}; // Pitch, roll, yaw, throttle
    static char text[TEXT_LEN] = "Messages output:\n";

    // Struct to pass the pointers
    pointers pointers;
    pointers.text = text;
    pointers.motorValues = motorValues;
    pointers.ackMode = &ackMode;
    pointers.gains = gains;

    // Open /dev/ttyUSB0
	serial_port_open(SERIAL_PORT);

	// Open joystick -> in debug mode we can continue without a joystick
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
    i16_to_ui8(gains[0], &config[0]);
    i16_to_ui8(gains[1], &config[2]);
    i16_to_ui8(gains[2], &config[4]);
    i16_to_ui8(gains[3], &config[6]);
	packMessage(CFG, config);

    // Set before time
	struct timeval tval_before, tval_after, tval_result;
	gettimeofday(&tval_before, NULL);

    // Start thread
    gui = std::thread(guiThread);
    
    // Open TCP socket -> for processing
	openSocket();

    // Variables for short-term usage
    int res;
	char c;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Structure which we can be pushed into the queue
        toGUI dataToSendGUI;

        // Process keys (like in pc_terminal)
        if ((c = term_getchar_nb()) != -1)
		{
            int8_t ret = processKeyboard(c, pilotCmd);
			if(ret != -1) dataToSendGUI.reqMode = ret;
		}

        // We set it to 9 as it is not a possible mode. If it stays 9, there was no mode request from the GUI side
        uint8_t modeChgFromGui = 9;
        // Get items from queue
        {
            // Lock with lock_guard
            std::lock_guard<std::mutex> myLock(syncToTerm);

            while (!qToTerm.empty())
            {
                toTerm rec = qToTerm.front();
                if (rec.reqMode != -1) modeChgFromGui = (uint8_t)rec.reqMode;
                if (rec.chgP    ==  1)  pilotCmd[1] |= (0x01 << 5); // U key
                if (rec.chgP    == -1)  pilotCmd[1] |= (0x01 << 4); // J key
                if (rec.chgP1   ==  1)  pilotCmd[1] |= (0x01 << 3); // I key
                if (rec.chgP1   == -1)  pilotCmd[1] |= (0x01 << 2); // K key
                if (rec.chgP2   ==  1)  pilotCmd[1] |= (0x01 << 1); // O key
                if (rec.chgP2   == -1)  pilotCmd[1] |=  0x01;       // L key
                if (rec.chgHei  ==  1)  pilotCmd[6] |= (0x01 << 1); // Y key
                if (rec.chgHei  == -1)  pilotCmd[6] |=  0x01;       // H key
                qToTerm.pop();
            }
            // Unlocks as lock_guard goes out of scope
        }

        // Send the mode change to the drone and set the requested mode
        if (modeChgFromGui < 8) 
        {
            if (get_fd_serial() == -1) serial_port_open(SERIAL_PORT);
            packMessage(MODE, &modeChgFromGui);
            dataToSendGUI.reqMode = modeChgFromGui;
        }
        else if (modeChgFromGui == 8)
        {
            // Open bluetooth port
		    ble_port_open(BT_PORT);

            // Change to wireless mode if bluetooth opened successfully
            if (get_fd_ble() != -1)
            {
                packMessage(MODE, &modeChgFromGui);
                dataToSendGUI.reqMode = modeChgFromGui;
            }
            else
            {
                printf("Couldn't change to wireless mode. No bluetooth connection.\n");
            }
        }
        

        // Check elapsed time -> for periodic sending
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = ((double)tval_result.tv_sec*1000.0f) + ((double)tval_result.tv_usec/1000.0f);

        // Send if more time elapsed then SEND_RATE_MS
        if (time_elapsed > SEND_RATE_MS)
        {
            // Read joystick axes and buttons
            if(joystickFound) {
                getJoyValues(pilotCmd);
            }

            // Map into symmetric range for GUI
            // Pitch
            if (pilotCmd[3] == 127) joy[0] = 0.0f;
            else if (pilotCmd[3] < 127) joy[0] = (float)((pilotCmd[3] - 127) / 127.0f * 100.0f);
            else joy[0] = (float)((pilotCmd[3] - 127) / 128.0f * 100.0f);

            // Roll
            if (pilotCmd[2] == 127) joy[1] = 0.0f;
            else if (pilotCmd[2] < 127) joy[1] = (float)((pilotCmd[2] - 127) / 127.0f * 100.0f);
            else joy[1] = (float)((pilotCmd[2] - 127) / 128.0f * 100.0f);

            // Yaw
            if (pilotCmd[4] == 127) joy[2] = 0.0f;
            else if (pilotCmd[4] < 127) joy[2] = (float)((pilotCmd[4] - 127) / 127.0f * 100.0f);
            else joy[2] = (float)((pilotCmd[4] - 127) / 128.0f * 100.0f);

            // Throttle
            joy[3] = (float)(pilotCmd[5] / 255.0f * 100);

            for (int i = 0; i < 4; i++)
            {
                dataToSendGUI.joy[i] = joy[i];
            }       

            // Send the CMD message
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
        int finishedMsg = 0;
        int ret;
        while ((res = serial_port_getchar(UART)) != -1)
        {
            c = (char)res;
            ret = unpackMessageGui(c, &SSM, pointers);
            if (ret == '.') done = true;
            finishedMsg |= ret;
        }
        while ((res = serial_port_getchar(BLUETOOTH)) != -1)
        {
            c = (char)res;
            ret = unpackMessageGui(c, &BSM, pointers);
            if (ret == '.') done = true;
            finishedMsg |= ret;
        }

        // Update motorValues, ackMode, text, gains if at least a message was finished (saves lots of copy instructions)
        if (finishedMsg)
        {
            for (int i = 0; i < 4; i++)
            {
                dataToSendGUI.motorValues[i] = motorValues[i];
            }
            dataToSendGUI.ackMode = ackMode;
            strncpy(dataToSendGUI.text, text, TEXT_LEN);
            dataToSendGUI.p = gains[0];
            dataToSendGUI.p1 = gains[1];
            dataToSendGUI.p2 = gains[2];
            dataToSendGUI.hei = gains[3];
        }

        // Put items into queue
        {
            // Lock with lock_guard
            std::lock_guard<std::mutex> myLock(syncToGui);

            qToGUI.push(dataToSendGUI);

            // Unlocks as lock_guard goes out of scope
        }
        
    }

    // Close the serial and bluetooth port
    serial_port_close();
    ble_port_close();
    printf("Ports closed\n");


    // Close TCP socket
    closeSocket();

    // Waiting threads to finish
    if (gui.joinable()) 
    {
        printf("Waiting for GUI to be closed\n");
        gui.join();
    }

    term_exitio();
	term_puts("\n<exit>\n");
    return 0;
}
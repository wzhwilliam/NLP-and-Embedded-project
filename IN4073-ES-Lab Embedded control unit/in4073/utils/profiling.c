#include "profiling.h"
#include "timers.h"
#include "comm.h"

//Array that stores the start times for each profiling type:
uint32_t profilingStartTimes[ProfileTypes]; 

/**
 * @brief Set the initial values of the profiling data to 0.
 * 
 * @author Wesley de Hek
 * @param profileData - reference to object that contains the profiling data.
 */
void initProfiling(profilingData *profileData) {
    for(int i = 0; i < ProfileTypes; i++) {
        profileData->profilingData[i] = 0;
        profileData->profilingDataCnt[i] = 0;
    }
}

/**
 * @brief Start the profiling of a specific type, setting its start time.
 * 
 * @author Wesley de Hek
 * @param type - Profiling type
 * @return true - Profiling successfully started.
 * @return false - Profiling not started.
 */
bool startProfiling(enum ProfileType type) {
    //Checking if type exists:
    if((type < 0) || (type >= ProfileTypes)) {
        char msg[100];
        snprintf(msg, 100, "Cannot start profiling for type %d, type does not exist.", type);
        packMessage(DEBUG, NULL, msg);

        return false;
    }

    //Checking if profile is not already running for this type:
    if(profilingStartTimes[type] != 0) {
        char msg[100];
        snprintf(msg, 100, "Cannot start profiling for type %d, aleady running.", type);
        packMessage(DEBUG, NULL, msg); 

        return false;    
    }

    //Saving start time:
    profilingStartTimes[type] = get_time_us();

    return true;
}

/**
 * @brief Stops the profiling of a specific type.
 * 
 * @author Wesley de Hek
 * @param type - Profiling type
 * @param saveData - Save data, so that it can be used to get the average later
 * @param data - reference to object that contains the profiling data.
 * @return uint32_t - the total time between start and end of profiling, this time is also saved to a buffer that can be used to calculate the average.
 */
uint32_t stopProfiling(enum ProfileType type, bool saveData, profilingData *data) {
    //Checking if type exists:
    if(type < 0 || type > ProfileTypes) {
        char msg[100];
        snprintf(msg, 100, "Cannot stop profiling for type %d, type does not exist.", type);
        packMessage(DEBUG, NULL, msg);

        return 0;
    }

    uint32_t startTime = profilingStartTimes[type];

    //Checking if profiling was started:
    if(startTime == 0) {
        char msg[100];
        snprintf(msg, 100, "Cannot stop profiling for type %d, no profile was running for this type.", type);
        packMessage(DEBUG, NULL, msg);  

        return 0;    
    }

    uint32_t endTime = get_time_us();
    uint32_t timeDiff = endTime - startTime;

    //Saving it:
    // if(saveData) {
    //     data->profilingData[type] += timeDiff;
    //     data->profilingDataCnt[type]++;
    // }

    //Setting start time back to 0:
    profilingStartTimes[type] = 0;

    //Returning it back for 
    return timeDiff;
}

/**
 * @brief Get the Average Profiling Time object
 * 
 * @author Wesley de Hek
 * @param type - Profiling type
 * @param data - reference to object that contains the profiling data.
 * @return uint32_t - Average profiling time since last time average got retreived
 */
uint32_t getAverageProfilingTime(enum ProfileType type, profilingData *data) {
    if(data->profilingDataCnt[type] <= 0) {
        return 0;
    }

    uint32_t average = data->profilingData[type] / data->profilingDataCnt[type];

    // char msg[100];
    //     snprintf(msg, 100, "Profilingdata (%d): %ld, %d",  type, data->profilingData[type], data->profilingDataCnt[type]);
    //     packMessage(DEBUG, NULL, msg);

    data->profilingData[type] = 0;
    data->profilingDataCnt[type] = 0;

    return average;
}

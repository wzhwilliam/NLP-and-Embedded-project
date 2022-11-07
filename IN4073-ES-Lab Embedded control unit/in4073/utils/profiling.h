#ifndef PROFILING_H__
#define PROFILING_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#define ProfileTypes 8 
#define ProfileHistorySizeMax 64

enum ProfileType {
	p_ControlLoop, //around 2000 times per timer_flag
    p_YawMode,
    p_FullControl,
    p_RawMode,
    p_HeightMode,
    p_Logging,
    p_Sqrt,
    p_Timer_Flag
};

typedef struct  {
    uint32_t profilingData[ProfileTypes];
    uint16_t profilingDataCnt[ProfileTypes];
} profilingData;

void initProfiling(profilingData *profileData);
bool startProfiling(enum ProfileType type);
uint32_t stopProfiling(enum ProfileType type, bool saveData, profilingData *data);
uint32_t getAverageProfilingTime(enum ProfileType type, profilingData *data);

#endif /* PROFILING_H__ */
#ifndef TOOLS_H__
#define TOOLS_H__

#include <inttypes.h>
#include <stdbool.h>

uint32_t calculateAverage_unsigned(uint32_t *data, uint16_t nrOfItems);
int32_t calculateAverage(int32_t *data, uint16_t nrOfItems);

#endif
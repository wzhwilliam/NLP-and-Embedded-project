#include "tools.h"

/**
 * @brief Calculate the average of a given data set, for unsigned 32 bit integers
 * 
 * @author Wesley de Hek
 * @param data Array of values to be averaged
 * @param nrOfItems Total nr of items in the array
 * @return uint32_t average of the dataset
 */
uint32_t calculateAverage_unsigned(uint32_t *data, uint16_t nrOfItems) {
    uint32_t total = 0;
    uint16_t n = 1;

    for(int i = 0; i < nrOfItems; i++) {
        total = (((n-1)/n)*total) + data[i]/n;
        n++;
    }

    return total;
}

/**
 * @brief Calculate the average of a given data set, for signed 32 bit integers
 * 
 * @author Wesley de Hek
 * @param data Array of values to be averaged
 * @param nrOfItems Total nr of items in the array
 * @return int32_t average of the dataset
 */
int32_t calculateAverage(int32_t *data, uint16_t nrOfItems) {
    int32_t total = 0;
    uint16_t n = 1;

    for(int i = 0; i < nrOfItems; i++) {
        total = (((n-1)/n)*total) + data[i]/n;
        n++;
    }

    return total;
}
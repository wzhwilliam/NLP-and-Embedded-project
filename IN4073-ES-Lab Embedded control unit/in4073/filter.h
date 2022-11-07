#ifndef _H_FILTER
#define _H_FILTER

#include <inttypes.h>
#include <stdbool.h>

extern float BW_COEFf_Height_Y_i;
extern float BW_COEFf_Height_Y_i1;

extern float BW_COEFf_Raw_Y_i;// 1.158
extern float BW_COEFf_Raw_Y_i1;// -0.842

typedef struct {
  int16_t x_old;
  int16_t y_old;
} ButterWorthState_t;

typedef struct {
  int16_t b;
  int16_t phi;
} KalmanState_t;

uint16_t butterworth(ButterWorthState_t *state, int16_t x, float coeff_yi, float coeff_yi1);
void initKalmanState(KalmanState_t *state);
void kalman(int16_t *result_p, int16_t *result_phi, KalmanState_t *state, int16_t accelAngle, int16_t gyroAngle);


#endif
#include "filter.h"

#include "mpu6050/mpu6050.h"
#include "hal/timers.h"

// x-axis towards forward
// y-axis towards left
// z-axis towards up???


// https://www.meme.net.au/butterworth.html
// 1st order low pass fs=20 fc=41
// 1st order low pass fs=20 fc=1
// #define BW_COEFf_Y_i 7.314
// #define BW_COEFF_Y_i1 5.314

// // 1st order low pass fd = 20 fc=49 steep
// #define BW_COEFf_Y_i 1158/1000 // 1.158
// #define BW_COEFF_Y_i1 -842/1000 // -0.842

float BW_COEFf_Height_Y_i = 7.314;
float BW_COEFf_Height_Y_i1 = 5.314;

float BW_COEFf_Raw_Y_i = 2.376;
float BW_COEFf_Raw_Y_i1 = 0.376;

/**
 * @brief Executes one iteration of the butterworth filter
 * 
 * @author Philip Groet
 */
uint16_t butterworth(ButterWorthState_t *state, int16_t x, float coeff_yi, float coeff_yi1) {
    // Init state when not done yet
  if(state->x_old == 0 || state->y_old == 0) {
    state->x_old = x;
    state->y_old = x;
  }

  uint16_t y = ((x + state->x_old) + coeff_yi1 * state->y_old) / coeff_yi;
  state->x_old = x;
  state->y_old = y;

  return y;
}

#define KALMAN_P2PHI 100 //(1/(TIMER_PERIOD/1000)) // Compiler will optimize... I hope... It did not
#define KALMAN_C1 50 //2
#define KALMAN_C2 50000 // (KALMAN_C1/1100)
/**
 * @author Philip Groet
 */
void initKalmanState(KalmanState_t *state) {
  state->b = 0;
  state->phi = 0;
}

/**
 * @brief Executes one iteration of the kalman filter
 * 
 * @author Philip Groet
 */
void kalman(int16_t *result_p, int16_t *result_phi, KalmanState_t *state, int16_t accelAngle, int16_t gyroAngle) {
  int16_t p, e;

  p = gyroAngle - state->b; // estimate real p, subtract bias
  state->phi = state->phi + p/KALMAN_P2PHI; // predict phi
  e = state->phi - accelAngle; // compare to measured phi
  state->phi = state->phi - e/KALMAN_C1; // correct phi to some extent
  state->b = state->b + (e/KALMAN_P2PHI) / KALMAN_C2; // adjust bias term

  *result_p = p;
  *result_phi = state->phi; 
} 
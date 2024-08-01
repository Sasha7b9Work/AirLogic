#ifndef KALMAN_H
#define KALMAN_H
#include <math.h>
#include <wiring.h>

class KalmanFilter {

public:
  KalmanFilter(float mea_e, float est_e, float q);
  float updateEstimate(float mea);
  void setMeasurementError(float mea_e);
  void setEstimateError(float est_e);
  void setProcessNoise(float q);
  float getKalmanGain();
  float getEstimateError();

private:
  float _err_measure = 0;
  float _err_estimate;
  float _q;
  /* initial stuff */
  float _current_estimate = 0.001;
  float _last_estimate = 0.001;
  float _kalman_gain = 0.001;
};
#endif
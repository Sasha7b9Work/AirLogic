#ifndef KALMAN_H
#define KALMAN_H
#include <math.h>
#include <wiring.h>

class KalmanFilter {

public:
  KalmanFilter(double mea_e, double est_e, double q);
  double updateEstimate(double mea);
  void setMeasurementError(double mea_e);
  void setEstimateError(double est_e);
  void setProcessNoise(double q);
  double getKalmanGain();
  double getEstimateError();

private:
  double _err_measure;
  double _err_estimate;
  double _q;
  double _current_estimate;
  double _last_estimate;
  double _kalman_gain;
};
#endif
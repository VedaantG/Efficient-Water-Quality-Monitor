#ifndef CALIBRATION_H
#define CALIBRATION_H

float obtain_ph_calibration_val(int pin);
float obtain_turbidity_calibration_val(int pin,float ntu);
float obtain_slope_params_ph(float v1,float v2,float ph1,float ph2);
float obtain_slope_params_turbidity(float tur1, float tur2,float tur3,float v1,float v2,float v3);

#endif
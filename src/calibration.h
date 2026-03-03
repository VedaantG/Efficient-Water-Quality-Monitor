#ifndef CALIBRATION_H
#define CALIBRATION_H

struct linear_params{
    bool valid;
    float m;
    float b;
};

struct quad_params{
    bool valid;
    float a;
    float b;
    float c;
};


float obtain_ph_calibration_val(int pin);
float obtain_turbidity_calibration_val(int pin);
linear_params obtain_slope_params_ph(float v1,float v2,float ph1,float ph2);
quad_params obtain_slope_params_turbidity(float tur1, float tur2,float tur3,float v1,float v2,float v3);

#endif
#include <Arduino.h>
#include "calibration.h"

float obtain_ph_calibration_val(int pin){
    float ph_voltage_adc_sum = 0.0;
    for(int i = 0;i<500;i++){
        ph_voltage_adc_sum += analogRead(pin)*(3.3/4095);
        delayMicroseconds(50);
    }
    float ph_voltage_adc = ph_voltage_adc_sum/500;
    return ph_voltage_adc;
}

float obtain_turbidity_calibration_val(int pin,float ntu){
    float turbidity_voltage_adc_sum =0.0;
    for(int j=0;j<500;j++){
        turbidity_voltage_adc_sum += analogRead(pin)*(3.3/4095);
        delayMicroseconds(50); 
    }
    float turbidity_voltage_adc = turbidity_voltage_adc_sum/500;
    return turbidity_voltage_adc;
}

float obtain_slope_params_ph(float v1,float v2,float ph1,float ph2){
    float m,b;
    m = ((ph2 - ph1)/(v2 - v1));
    b = ph1 - (m*v1);
    return m,b;
}

float obtain_slope_params_turbidity(float tur1, float tur2,float tur3,float v1,float v2,float v3){
    float det = (v1*v1*(v2-v3)) - (v1*((v2*v2)-(v3*v3))) + ((v3*v2*v2)-(v3*v3*v2));
    if (det == 0){
        return NAN;
    }
    float c11,c21,c31;
    c11 = (tur1*(v2-v3)) + (tur2*(v3-v1)) + (tur3*(v1-v2));
    c21 = (tur1*((v3*v3)-(v2*v2))) + (tur2*((v1*v1)-(v3*v3))) + (tur3*((v2*v2)-(v1*v1)));
    c31 = (tur1*((v3*v2*v2)-(v3*v3*v2))) + (tur2*((v3*v3*v1)-(v3*v1*v1))) + (tur3*((v2*v1*v1)-(v2*v2*v1)));

    c11 = c11/det;
    c21 = c21/det;
    c31 = c31/det;

    return c11,c21,c31; //a,b,c
}
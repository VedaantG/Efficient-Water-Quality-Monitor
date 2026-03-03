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

float obtain_turbidity_calibration_val(int pin){
    float turbidity_voltage_adc_sum =0.0;
    for(int j=0;j<500;j++){
        turbidity_voltage_adc_sum += analogRead(pin)*(3.3/4095);
        delayMicroseconds(50); 
    }
    float turbidity_voltage_adc = turbidity_voltage_adc_sum/500;
    return turbidity_voltage_adc;
}

linear_params obtain_slope_params_ph(float v1,float v2,float ph1,float ph2){
    linear_params p;
    if(abs(v2-v1)<0.0008) {
        p.valid = false;
        p.m = NAN;
        p.b = NAN;
        return p;
    }
    p.m = ((ph2 - ph1)/(v2 - v1));
    p.b = ph1 - (p.m*v1);
    p.valid = true;
    return p;
}

quad_params obtain_slope_params_turbidity(float tur1, float tur2,float tur3,float v1,float v2,float v3){
    quad_params q;
    float det = (v1*v1*(v2-v3)) - (v1*((v2*v2)-(v3*v3))) + ((v3*v2*v2)-(v3*v3*v2));
    if (fabs(det) < 1e-6){
        q.a = NAN;
        q.b = NAN;
        q.c = NAN;
        q.valid = false;
        return q;
    }
    q.a = (tur1*(v2-v3)) + (tur2*(v3-v1)) + (tur3*(v1-v2));
    q.b = (tur1*((v3*v3)-(v2*v2))) + (tur2*((v1*v1)-(v3*v3))) + (tur3*((v2*v2)-(v1*v1)));
    q.c = (tur1*((v3*v2*v2)-(v3*v3*v2))) + (tur2*((v3*v3*v1)-(v3*v1*v1))) + (tur3*((v2*v1*v1)-(v2*v2*v1)));

    q.a = q.a/det;
    q.b = q.b/det;
    q.c = q.c/det;
    q.valid = true;

    return q; //a,b,c
}
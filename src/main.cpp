#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "calibration.h"


const int ONE_WIRE_BUS = 4;
const int ph_pin = 34;
const int turbidity_pin = 35;
const int CAL_BTN = 6;

//global parameters
float ph_slope = 0.0,ph_bias = 0.0,tur_coeff_1 = 0.0,tur_coeff_2 = 0.0,tur_coeff_3 = 0.0;

//deep sleep params
#define us_to_s 1000000ULL
// 5x60 = 300 (5 minutes to seconds) 
#define time_to_sleep 60 //60 for testing purposes

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float phTask(int ph_pin){
  float ph_voltage_adc = 0.0;
  float previous_reading = analogRead(ph_pin);
  int spikeCount = 0;
  float current_reading;
  for(int i=0; i<20;i++){
    current_reading = analogRead(ph_pin);
    ph_voltage_adc += current_reading;
    if(previous_reading > 0 && abs(current_reading - previous_reading)>(0.2*previous_reading)){
      spikeCount++;
    }
    else{
      spikeCount = 0;
    }
    previous_reading = current_reading;
    delay(100);
  }
  if (spikeCount>3){
    return NAN;
  }
  float ph_voltage = ph_voltage_adc*(3.3/(4095*20));
  float ph_value = (ph_slope*ph_voltage) + ph_bias;
  if(ph_value < 0 || ph_value > 14){
    return NAN;
  }
  return ph_value;
}

float Turbidity(int turbidity_pin){
  float tubrbidity_voltage_adc = 0.0;
  int spikeCount = 0;
  float previous_reading = analogRead(turbidity_pin);
  float current_reading;
  for(int j=0;j<20;j++){
    current_reading = analogRead(turbidity_pin);
    tubrbidity_voltage_adc += current_reading;
    if(previous_reading > 0 && abs(current_reading - previous_reading)>(0.2*previous_reading)){
      spikeCount++;
    }
    else{
      spikeCount = 0.0;
    }
    previous_reading = current_reading;
    delay(100);
  }
  if(spikeCount > 3){
    return NAN;
  }
  float turbidity_voltage = tubrbidity_voltage_adc*(3.3/(4095*20));
  float turbidity_NTU = (tur_coeff_1*turbidity_voltage*turbidity_voltage)+(tur_coeff_2*turbidity_voltage)+tur_coeff_3;
  if (turbidity_NTU < 0){
    return NAN;
  }
  return turbidity_NTU;
}

float Tempreature(){
  sensors.requestTemperatures();
  float sum = 0.0;
  int validCount = 0;
  for(int k=0;k<20;k++){
    float tempSum = sensors.getTempCByIndex(0);
    if(tempSum != DEVICE_DISCONNECTED_C){
      sum += tempSum;
      validCount++;
    }
    delay(100);
  }
  if (validCount == 0){
    return NAN;
  }
  return sum/validCount;
  
}

void initHardware(){
  Serial.begin(115200);
  sensors.begin();
  pinMode(ph_pin,INPUT);
  pinMode(turbidity_pin,INPUT);
  pinMode(CAL_BTN,INPUT_PULLUP);
  analogReadResolution(12); //2^12 = 4096
  analogSetAttenuation(ADC_11db); //11db will read from ~0 to ~3.3v 
}

void do_calibration(){
  float ph_voltage_adc_ph7 = 0.0;
  float ph_voltage_adc_ph4 = 0.0;
  float turbidity_voltage_adc_0 = 0.0;
  float turbidity_voltage_adc_100 = 0.0;
  float turbidity_voltage_adc_500 = 0.0;
  float turbidity_voltage_adc = obtain_turbidity_calibration_val(turbidity_pin);
  Serial.println("Dip PH sensor in PH 4 Buffer Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(true){
    if(digitalRead(CAL_BTN) == LOW){
      ph_voltage_adc_ph4 = obtain_ph_calibration_val(ph_pin);
      break;
    }
  }
  Serial.println("Dip PH sensor in PH 7 Buffer Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(true){
    if(digitalRead(CAL_BTN) == LOW){
      ph_voltage_adc_ph7 = obtain_ph_calibration_val(ph_pin);
      break;
    }
  }
  Serial.println("Dip Turbidity sensor in NTU ~0 (Distilled water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(true){
    if(digitalRead(CAL_BTN) == LOW){
      turbidity_voltage_adc_0 = obtain_turbidity_calibration_val(turbidity_pin);
      break;
    }
  }
  Serial.println("Dip Turbidity sensor in NTU ~100 (Cloudy Water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(true){
    if(digitalRead(CAL_BTN) == LOW){
      turbidity_voltage_adc_100 = obtain_turbidity_calibration_val(turbidity_pin);
      break;
    }
  }
  Serial.println("Dip Turbidity sensor in NTU ~500 (Muddy Water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(true){
    if(digitalRead(CAL_BTN) == LOW){
      turbidity_voltage_adc_500 = obtain_turbidity_calibration_val(turbidity_pin);
      break;
    }
  }
  linear_params ph_params = obtain_slope_params_ph(ph_voltage_adc_ph7,ph_voltage_adc_ph4,7,4);
  quad_params turbidity_params = obtain_slope_params_turbidity(0,100,500,turbidity_voltage_adc_0,turbidity_voltage_adc_100,turbidity_voltage_adc_500);
  ph_slope = ph_params.m;
  ph_bias = ph_params.b;
  tur_coeff_1 = turbidity_params.a;
  tur_coeff_2 = turbidity_params.b;
  tur_coeff_3 = turbidity_params.c;
}

void setup() {
  initHardware();
  if(digitalRead(CAL_BTN)== LOW){
    do_calibration();
  }
  Serial.println("____________Water Quality Monitor____________");
  float ph = phTask(ph_pin);
  float turbidity = Turbidity(turbidity_pin);
  float tempreatureC = Tempreature();
  Serial.print("PH: ");
  Serial.println(ph);
  Serial.print("Turbidity: ");
  Serial.println(turbidity);
  Serial.print("Tempreature: ");
  Serial.println(tempreatureC);
  delay(100);

  //deep sleep
  esp_sleep_enable_timer_wakeup(time_to_sleep*us_to_s);
  delay(100);
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}

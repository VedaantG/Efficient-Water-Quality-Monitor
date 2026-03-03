#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "calibration.h"


const int ONE_WIRE_BUS = 4;
const int ph_pin = 34;
const int turbidity_pin = 35;
const int CAL_BTN = 6;

Preferences prefs;

//global parameters
float ph_slope = 0.0,ph_bias = 0.0,tur_coeff_1 = 0.0,tur_coeff_2 = 0.0,tur_coeff_3 = 0.0;
bool is_valid = false;

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
      spikeCount = 0;
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

float Temperature(){
  float sum = 0.0;
  int validCount = 0;
  for(int k=0;k<20;k++){
    sensors.requestTemperatures();
    delay(10); //sensor takes 750ms to retrive values
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

void deep_sleep_begin(){
  esp_sleep_enable_timer_wakeup(time_to_sleep*us_to_s);
  delay(100);
  Serial.flush();
  esp_deep_sleep_start();
}

void store_vals_to_nvs(float ph_slope,float ph_bias,float tur_a,float tur_b,float tur_c){
  prefs.begin("calib",false); //false -> read/write enable
  prefs.putFloat("ph_m",ph_slope);
  prefs.putFloat("ph_b",ph_bias);
  prefs.putFloat("tur_coeff_a",tur_a);
  prefs.putFloat("tur_coeff_b",tur_b);
  prefs.putFloat("tur_coeff_c",tur_c);
  prefs.putBool("cal_done",true);
  prefs.end();
}

void load_vals_from_nvs(){
  prefs.begin("calib",true);
  is_valid = prefs.getBool("cal_done",false);
  if(is_valid){
    ph_slope = prefs.getFloat("ph_m",0.0);
    ph_bias = prefs.getFloat("ph_b",0.0);
    tur_coeff_1 = prefs.getFloat("tur_coeff_a",0.0);
    tur_coeff_2 = prefs.getFloat("tur_coeff_b",0.0);
    tur_coeff_3 = prefs.getFloat("tur_coeff_c",0.0);
  }
  prefs.end();
  if(abs(ph_slope)<0.001){
    is_valid = false;
  }
}

void do_calibration(){
  float ph_voltage_adc_ph7 = 0.0;
  float ph_voltage_adc_ph4 = 0.0;
  float turbidity_voltage_adc_0 = 0.0;
  float turbidity_voltage_adc_100 = 0.0;
  float turbidity_voltage_adc_500 = 0.0;
  Serial.println("Dip PH sensor in PH 4 Buffer Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(digitalRead(CAL_BTN) == HIGH){
    delay(10);
  }
  ph_voltage_adc_ph4 = obtain_ph_calibration_val(ph_pin);
  delay(50);
  Serial.println("Dip PH sensor in PH 7 Buffer Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(digitalRead(CAL_BTN) == HIGH){
    delay(10);
  }
  ph_voltage_adc_ph7 = obtain_ph_calibration_val(ph_pin);
  delay(50);
  Serial.println("Dip Turbidity sensor in NTU ~0 (Distilled water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(digitalRead(CAL_BTN) == HIGH){
    delay(10);
  }
  turbidity_voltage_adc_0 = obtain_turbidity_calibration_val(turbidity_pin);
  delay(50);
  Serial.println("Dip Turbidity sensor in NTU ~100 (Cloudy Water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(digitalRead(CAL_BTN) == HIGH){
    delay(10);
  }
  turbidity_voltage_adc_100 = obtain_turbidity_calibration_val(turbidity_pin);
  delay(50);
  Serial.println("Dip Turbidity sensor in NTU ~500 (Muddy Water) Solution");
  Serial.println("Press Any key after Dipping sensor");
  while(digitalRead(CAL_BTN) == HIGH){
    delay(10);
  }
  turbidity_voltage_adc_500 = obtain_turbidity_calibration_val(turbidity_pin);
  delay(50);
  linear_params ph_params = obtain_slope_params_ph(ph_voltage_adc_ph7,ph_voltage_adc_ph4,7,4);
  quad_params turbidity_params = obtain_slope_params_turbidity(0,100,500,turbidity_voltage_adc_0,turbidity_voltage_adc_100,turbidity_voltage_adc_500);
  if(!ph_params.valid){
    ph_slope = NAN;
    ph_bias = NAN;
  }
  else{
    ph_slope = ph_params.m;
    ph_bias = ph_params.b;
  }
  if(!turbidity_params.valid){
    tur_coeff_1 = NAN;
    tur_coeff_2 = NAN;
    tur_coeff_3 = NAN;
  }
  else{
    tur_coeff_1 = turbidity_params.a;
    tur_coeff_2 = turbidity_params.b;
    tur_coeff_3 = turbidity_params.c;
  }
  //saving calculated konstants to non volatile storage
  if(ph_params.valid && turbidity_params.valid){
    store_vals_to_nvs(ph_slope,ph_bias,tur_coeff_1,tur_coeff_2,tur_coeff_3);
  }
  else{
    Serial.println("Calibration Failed :(");
  }
}

void NORMAL_MODE(){
  Serial.println("____________Water Quality Monitor____________");
  float ph = phTask(ph_pin);
  float turbidity = Turbidity(turbidity_pin);
  float tempreatureC = Temperature();
  Serial.print("PH: ");
  Serial.println(ph);
  Serial.print("Turbidity: ");
  Serial.println(turbidity);
  Serial.print("Tempreature: ");
  Serial.println(tempreatureC);
  delay(100);
}

void setup() {
  initHardware();
  //load values from nvs
  load_vals_from_nvs();
  if(digitalRead(CAL_BTN)== LOW || !is_valid){//CAL_MODE
    do_calibration();
  }
  
  NORMAL_MODE();

  //deep sleep
  deep_sleep_begin();
}

void loop() {}

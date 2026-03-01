#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>


const int ONE_WIRE_BUS = 4;
const int ph_pin = 34;
const int turbidity_pin = 35;

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
    ph_voltage_adc += analogRead(ph_pin);
    current_reading = ph_voltage_adc;
    if(previous_reading > 0 && abs(current_reading - previous_reading)>(0.2*previous_reading)){
      spikeCount++;
    }
    else{
      spikeCount = 0.0;
    }
    previous_reading = current_reading;
    delay(2);
  }
  if (spikeCount>3){
    return NAN;
  }
  float ph_voltage = ph_voltage_adc*(3.3/(4095*20));
  //change the values after calibration
  float ph_value = (3.0*ph_voltage) + 0.5;
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
    tubrbidity_voltage_adc += analogRead(turbidity_pin);
    current_reading = tubrbidity_voltage_adc;
    if(previous_reading > 0 && abs(current_reading - previous_reading)>(0.2*previous_reading)){
      spikeCount++;
    }
    else{
      spikeCount = 0.0;
    }
    previous_reading = current_reading;
    delay(2);
  }
  if(spikeCount > 3){
    return NAN;
  }
  float turbidity_voltage = tubrbidity_voltage_adc*(3.3/(4095*20));
  //change this values after calibration
  float turbidity_NTU = (-1120.4*turbidity_voltage*turbidity_voltage)+(5742.3*turbidity_voltage)-4352.9;
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
    delay(1);
  }
  if (validCount == 0){
    return NAN;
  }
  return sum/validCount;
  
}

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Serial.println("____________Water Quality Monitor____________");
  pinMode(ph_pin,INPUT);
  pinMode(turbidity_pin,INPUT);


  float ph = phTask(ph_pin);
  float turbidity = Turbidity(turbidity_pin);
  float tempreatureC = Tempreature();
  Serial.print("PH: ");
  Serial.println(ph);
  Serial.print("Turbidity: ");
  Serial.println(turbidity);
  Serial.print("Tempreature: ");
  Serial.println(tempreatureC);
  delay(1000);


  //deep sleep
  esp_sleep_enable_timer_wakeup(time_to_sleep*us_to_s);
  delay(100);
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}

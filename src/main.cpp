#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>


const int ONE_WIRE_BUS = 4;
const int ph_pin = 34;
const int turbidity_pin = 35;

//deep sleep params
#define us_to_s 1000000ULL
// 5x60 = 300 (5 minutes to seconds) 
#define time_to_sleep 60

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float phTask(int ph_pin){
  float ph_voltage_adc = analogRead(ph_pin);
  float ph_voltage = ph_voltage_adc*(3.0/4096);
  //change the values after calibration
  float ph_value = (3.0*ph_voltage) + 0.5;
  return ph_value;
}

float Turbidity(int turbidity_pin){
  float tubrbidity_voltage_adc = analogRead(turbidity_pin);
  float turbidity_voltage = tubrbidity_voltage_adc*(3.3/4096);
  //change this values after calibration
  float turbidity_NTU = (-1120.4*turbidity_voltage*turbidity_voltage)+(5742.3*turbidity_voltage)-4352.9;
  return turbidity_NTU;
}

float Tempreature(){
  sensors.begin();
  delay(1);
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  return tempC;
}

void setup() {
  Serial.begin(115200);
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
  esp_deep_sleep_start();
}

void loop() {}

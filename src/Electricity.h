#ifndef ELECTRICITY_H
#define ELECTRICITY_H

#include <Adafruit_INA219.h>
//#include <Wire.h>

float shuntvoltage;
float busvoltage;
float current_mA;
float loadvoltage;
float power_mW;
float power_W;
const float shunt_resistor = 0.002; // 2mOhm shunt resistor

Adafruit_INA219 ina219(0x45);

class Electricity {

public:
  static void setup() {
    ina219.begin();
    ina219.setCalibration_32V_2A();
  }

  int getVoltage() {
    return busvoltage;
  }

  int getCurrent() {
    return current_mA;
  }

  int getPower() {
    return power_mW; // returns avg system power in mW
  }

  void updateApp() {
   // Blynk.virtualWrite(V43, busvoltage);
  //  Blynk.virtualWrite(V44, current_mA);
   // Blynk.virtualWrite(V45, power_W);
  }

  void loop() {

    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();

    current_mA = shuntvoltage / shunt_resistor;
    power_mW = current_mA * busvoltage;

    current_mA = current_mA / 1000;
    power_W = power_mW / 1000;

    //Blynk.virtualWrite(V43, busvoltage);
    //Blynk.virtualWrite(V44, current_mA);
    //Blynk.virtualWrite(V45, power_mW);
  }
};

#endif

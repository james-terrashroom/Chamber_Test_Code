#ifndef TEST_H
#define TEST_H

#include "LEDArray.h"
#include "Electricity.h"
#include "Test.h"
#include "Habitat.h"


// 0 is not tested / 1 is not working and tested / 2 is working and tested

Habitat habitat_check;
Electricity electricity_check;
int start_time = 0;
bool ended_test = true;

class Test {
public:
    Test() {
    }

 
  void run() {
    if(ended_test) {start_time = millis(); test_Number++; ended_test = false;}

    
    if(test_Number == 4) {
        ledcWrite(FAN_1_PWM, ON_DUTY);
    }
    if(test_Number == 5) {
        ledcWrite(FAN_2_PWM, ON_DUTY);
    }
    if(test_Number == 6) {
        ledcWrite(BLOWER_PWM, ON_DUTY);
    }
    if(test_Number == 8) {
        ledcWrite(ATOMIZER_PWM, ON_DUTY);
    }
    if(test_Number == 9) {
        digitalWrite(UV_LED, HIGH);
    }
    if(test_Number == 10) {
         ledcWrite(HEATER_PWM, 4095); 
    }


    if(test_Number == 1 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 3000 && electricity_check.getPower() < 9000) {
            Serial.println("Top Light Test: Passed");
            top_Light_Status = 2;
        }
        else {
            Serial.println("Top Light Test: Failed");
            top_Light_Status = 1;
        }
        ended_test = true;
    }

    if(test_Number == 2 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 9000 && electricity_check.getPower() < 16500) {
            Serial.println("Bottom Light Test: Passed");
            bottom_Light_Status = 2;
        }
        else {
            Serial.println("Bottom Light Test: Failed");
            bottom_Light_Status = 1;
        }
        ended_test = true;
    }

    if(test_Number == 3) {
        Serial.println("Test Number: " + String(test_Number));
        digitalWrite(WATER_READ_ENABLE, HIGH);
        delay(3);
        if(digitalRead(WATER_LOW)) {water_Sensor_Status = 2; Serial.println("Water Level Sensor Test: Passed"); ended_test = true; digitalWrite(WATER_READ_ENABLE, LOW);}
        else {water_Sensor_Status = 1; Serial.println("Water Level Sensor Test: Failed");}
    }

    if(test_Number == 4 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 4000 && electricity_check.getPower() < 8000) {
            Serial.println("Fan 1 Test: Passed");
            fan_1_Status = 2;
        }
        else {
            Serial.println("Fan 1 Test: Failed");
            fan_1_Status = 1;
        }
        ledcWrite(FAN_1_PWM, OFF_DUTY);
        ended_test = true;
    }

    if(test_Number == 5 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 4000 && electricity_check.getPower() < 8000) {
            Serial.println("Fan 2 Test: Passed");
            fan_2_Status = 2;
        }
        else {
            Serial.println("Fan 2 Test: Failed");
            fan_2_Status = 1;
        }
        ledcWrite(FAN_2_PWM, OFF_DUTY);
        ended_test = true;
    }

    if(test_Number == 6 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 450 && electricity_check.getPower() < 2000) {
            Serial.println("Blower Test: Passed");
            blower_Status = 2;
        }
        else {
            Serial.println("Blower Test: Failed");
            blower_Status = 1;
        }
        ledcWrite(BLOWER_PWM, OFF_DUTY);
        ended_test = true;
    }

    if(test_Number == 7) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        Serial.println("Test Number: " + String(test_Number));
        if(took_picture == 1) {
            Serial.println("Camera Test: Passed");
            camera_Status = 2;
            ended_test = true;
        }
        else {
            Serial.println("Camera Test: Failed");
            camera_Status = 1;
        }
        
    }

    if(test_Number == 8 && millis() - start_time > 1000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 20000 && electricity_check.getPower() < 40000) {
            Serial.println("Atomizer Test: Passed");
            atomizer_Status = 2;
        }
        else {
            Serial.println("Atomizer Test: Failed");
            atomizer_Status = 1;
        }
        ledcWrite(ATOMIZER_PWM, OFF_DUTY);
        ended_test = true;
    }

    if(test_Number == 9 && millis() - start_time > 1000) {
         Serial.println("Test Number: " + String(test_Number));
         Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() < 6000) {
            Serial.println("UV Test: Passed");
            uv_Status = 2;
        }
        else {
            Serial.println("UV Test: Failed");
            uv_Status = 1;
        }
        digitalWrite(UV_LED, LOW);
        ended_test = true;
    }

    if(test_Number == 10 && millis() - start_time > 5000) {
        Serial.println("Test Number: " + String(test_Number));
        Serial.println(electricity_check.getPower());
        if(electricity_check.getPower() > 50000) {
            Serial.println("Heater Test: Passed");
            heater_Status = 2;
        }
        else {
            Serial.println("Heater Test: Failed");
            heater_Status = 1;
        }
        ledcWrite(HEATER_PWM, OFF_DUTY);
        ended_test = true;
    }

  }

  void loop() {
     habitat_check.sensorCheck();

     if(habitat_check.tempActual < 195 && habitat_check.humidityActual <99) { hdc1080_Status = 2;}
     else {hdc1080_Status = 1;}

     electricity_check.loop();
     electricity_check.updateApp();
     if(electricity_check.getVoltage() > 22) {voltag_Status = 2;}
     else {voltag_Status = 1;}

  }
  void updateCameraTestResult(int result) {
    took_picture = result;
  }

int took_picture = 0;
int test_Number = 0;
int top_Light_Status = 0;
int bottom_Light_Status = 0;
int water_Sensor_Status = 0;
int fan_1_Status = 0;
int fan_2_Status = 0;
int blower_Status = 0;
int camera_Status = 0;
int heater_Status = 0;
int atomizer_Status = 0;
int uv_Status = 0;
int hdc1080_Status = 0;
int voltag_Status = 0;
bool ble_Status = 0;

private:


};
#endif
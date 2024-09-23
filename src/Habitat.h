#ifndef HABITAT_H
#define HABITAT_H

/*
NOTES: 

- for some reason fans turn on even when the setpoints are dead on?

*/

#include "ClosedCube_HDC1080.h"
//#include "Electricity.h"

ClosedCube_HDC1080 hdc1080; //  Temperature/Humidity Sensor

const float HUMIDITY_PCT_PER_SECOND = 7.5;

const int ON_DUTY = 256;
const int HALF_DUTY = 128;
const int OFF_DUTY = 0;

const u_int8_t HEATER_PIN = 12;
const u_int8_t FAN_1_PIN = 32; // FAN 1 is EXHAUST
const u_int8_t FAN_2_PIN = 33; // FAN 2 is INTAKE
const u_int8_t BLOWER_PIN = 25;
const u_int8_t ATOMIZER_PIN = 13;
const u_int8_t UV_LED = 18; // this pin (D18) controls the UV LED ON/OFF
const u_int8_t WATER_READ_ENABLE = 15;
const u_int8_t WATER_LOW = 23; // This will be LOW if the water level is below the set point

const u_int8_t FAN_1_PWM = 2;
const u_int8_t FAN_2_PWM = 3;
const u_int8_t BLOWER_PWM = 4;
const u_int8_t ATOMIZER_PWM = 5;
const u_int8_t HEATER_PWM = 6;


// PWM channel initialized at 24 Hz with a 8 bit resolution (cuts down on noise)
const int PWM_FREQ = 10000; //50 hz works really well for the fans | 22kHz is even better
const int PWM_RESOLUTION = 8;

// Define global variables to track the last notification time for each type
const int lastLowWaterNotificationTime = 3610;

// Define rate limit constants (in seconds)
int lowWaterNotificationRateLimit = 30; // One notification per hour

class Habitat {
private:
  // State Variables
  //int tempActual = 0;     // Actual Reported temperature value from HDC1080
  //int humidityActual = 0; // Actual Reported humidity value from HDC1080
  bool manual = true;     // Manual Mode for Terrashroom DEFAULTS TO TRUE FIXED
  int habitat_mode = 0;   // 0 = silent, 1 = performance 

  bool atomizerStarted = false;
  bool humidityDecreaseStarted = false;
  bool temperatureDecreaseStarted = false;
  bool temperatureIncreaseStarted = false;
  bool airExchangeStarted = false;
  bool resetHDC1080Started = false;

  int humidityIncreaseStart = INT_MIN;
  int humidityDecreaseStart = INT_MIN;
  int temperatureDecreaseStart = INT_MIN;
  int temperatureIncreaseStart = INT_MIN;
  int airExhangeStart = INT_MIN;
  int resetHDC1080Start = INT_MIN;
  int preventWaterDamageStart = INT_MIN;

  int hdcRetries = 0; // For retrying on failed HDC reads
  int waterlevelreadfreq = 0;

  // Failure mode variables
  int lastWaterLevelCall = 0;
  int lastWaterLevelNotif = 0;
  bool firstWaterLevelNotif = false;
  bool preventWaterDamage = false;
  bool hour_no_water = false;
  bool four_hours_no_water = false;
  bool day_no_water = false;
  bool first_temp_sensor_fail_notification = false;
  bool second_temp_sensor_fail_notification = false;
  bool third_temp_sensor_fail_notification = false;
  int temp_sensor_failure_time = INT_MIN;

  // Future use

  int waterConservation = 1;
  bool enabled = true;
  bool dryMode = false;
  bool preventHeatDamage = false;

  bool preventHumidityDamage = false;
  int dryingStart = 0;

public:
  Habitat() {
  }

  static void setup() {
    
    pinMode(HEATER_PIN, OUTPUT); // Declaring the Heater Drive Pin (pin 12) as an Output GPIO
    pinMode(FAN_1_PIN, OUTPUT);  // Declaring the Fan Drive Pin (pin 32) as an Output GPIO
    pinMode(FAN_2_PIN, OUTPUT);  // Declaring the Fan Drive Pin (pin 33) as an Output GPIO
    pinMode(BLOWER_PIN, OUTPUT); // Declaring the Blower Drive Pin (pin 25) as an Output GPIO
    pinMode(ATOMIZER_PIN, OUTPUT); // Declaring the Atomizer Drive Pin (pin 13) as an Output GPIO

    digitalWrite(HEATER_PIN, LOW); // Setting the Heater Drive Pin (pin 12) to LOW
    digitalWrite(FAN_1_PIN, LOW);  // Setting the Fan Drive Pin (pin 32) to LOW
    digitalWrite(FAN_2_PIN, LOW);  // Setting the Fan Drive Pin (pin 33) to LOW
    digitalWrite(BLOWER_PIN, LOW); // Setting the Blower Drive Pin (pin 25) to LOW
    digitalWrite(ATOMIZER_PIN, LOW); // Setting the Atomizer Drive Pin (pin 13) to LOW

    ledcSetup(FAN_1_PWM, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(FAN_2_PWM, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(ATOMIZER_PWM, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(BLOWER_PWM, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(HEATER_PWM, 400, 12);

    ledcAttachPin(FAN_1_PIN, FAN_1_PWM);
    ledcAttachPin(FAN_2_PIN, FAN_2_PWM);
    ledcAttachPin(BLOWER_PIN, BLOWER_PWM);
    ledcAttachPin(ATOMIZER_PIN, ATOMIZER_PWM);
    ledcAttachPin(HEATER_PIN, HEATER_PWM);

    ledcWrite(FAN_1_PWM, 8); // OFF_DUTY
    ledcWrite(FAN_2_PWM, 8); // OFF_DUTY
    ledcWrite(BLOWER_PWM, OFF_DUTY);
    ledcWrite(ATOMIZER_PWM, OFF_DUTY);
    ledcWrite(HEATER_PWM, OFF_DUTY);

    //pinMode(HEATER_PIN, OUTPUT); // Declaring the Heater Drive Pin (pin 12) as an Output GPIO
    pinMode(UV_LED, OUTPUT);     // Declaring the UV LED Drive Pin (pin 18) as an Output GPIO

    pinMode(WATER_LOW, INPUT_PULLDOWN); // Declaring the water low level sensor as an Input GPIO
    pinMode(WATER_READ_ENABLE, OUTPUT);

    digitalWrite(WATER_READ_ENABLE, LOW); // not reading in the water level

    hdc1080.begin(0x40); // Starting temp & humidity sensor

    //hdc1080.heatUp(10); //this might be blocking
  }

  int now() {
    // TODO: handle rollover
    return millis() / 1000;
  }

  int elapsed(int time) {
    return now() - time;
  }

  void carbonDioxideFlush() {
    // cycle air in, then exhaust
    // run blower to maintain humidity during cycle
    if(habitat_mode == 0) { // QUIET MODE
      ledcWrite(BLOWER_PWM, ON_DUTY);
      ledcWrite(FAN_2_PWM, 9); //intake
      delay(10000);
      ledcWrite(FAN_2_PWM, 8);
      ledcWrite(FAN_1_PWM, 13); //exhaust
      delay(20000);
      ledcWrite(FAN_1_PWM, 8); //exhaust
      ledcWrite(BLOWER_PWM, OFF_DUTY);
    }

    else { // PERFORMANCE MODE
      ledcWrite(BLOWER_PWM, ON_DUTY);
      ledcWrite(FAN_2_PWM, 20); //intake
      delay(10000);
      ledcWrite(FAN_2_PWM, 8); 
      ledcWrite(FAN_1_PWM, 25); //exhaust
      delay(20000);
      ledcWrite(FAN_1_PWM, 9);
      ledcWrite(BLOWER_PWM, OFF_DUTY);
    }
  }


  // External environment is TOO COLD
  // INCREASE chamber temp
  void handleTemperatureIncrease() {
    if(habitat_mode == 0) { //QUIET MODE

      if (tempSetPoint > (tempActual + 1) && !temperatureIncreaseStarted) { // beta tester feedback wants the temp to be exactly at set point & not below by the tolerance
        Serial.println("temperatureIncrease");
        temperatureIncreaseStart = now();
        ledcWrite(HEATER_PWM, 900); //"Soft Starting" the heater so that we can use the 5A good looking power supply and not the large power supply
        temperatureIncreaseStarted = true;
      } 

      if(temperatureIncreaseStarted && tempSetPoint > tempActual && elapsed(temperatureIncreaseStart) > 60) { //after 60 seconds of pre-heating, increase the heater power as to not overdraw the power supply
        ledcWrite(HEATER_PWM, 4095);
        ledcWrite(FAN_1_PWM, 7); //exhaust
        ledcWrite(FAN_2_PWM, 11); //intake
        
      }

      if(temperatureIncreaseStarted && tempSetPoint > tempActual && elapsed(temperatureIncreaseStart) > 300) { //after 300 seconds of heating, increase the fans
        ledcWrite(HEATER_PWM, 4095);
        ledcWrite(FAN_1_PWM, 8); // prev 20
        ledcWrite(FAN_2_PWM, 11);
        
      }

      if (tempSetPoint < (tempActual + 1) || elapsed(temperatureIncreaseStart) > 900) { // stop heating after 15 minutes if it still isn't going up
        temperatureIncreaseStarted = false;
        ledcWrite(FAN_1_PWM, 8); // these just need to set back to baseline airflow
        ledcWrite(FAN_2_PWM, 12);
        ledcWrite(HEATER_PWM, 0);
      
      }
    
    }

    if(habitat_mode == 1) { //PERFORMANCE MODE
      
      if (tempSetPoint > (tempActual + 1) && !temperatureIncreaseStarted) { // beta tester feedback wants the temp to be exactly at set point & not below by the tolerance
        Serial.println("temperatureIncrease");
        temperatureIncreaseStart = now();
        ledcWrite(HEATER_PWM, 900); //"Soft Starting" the heater so that we can use the 5A good looking power supply and not the large power supply
        temperatureIncreaseStarted = true;
      } 

      if(temperatureIncreaseStarted && tempSetPoint > tempActual && elapsed(temperatureIncreaseStart) > 60) { //after 60 seconds of pre-heating, increase the heater power as to not overdraw the power supply
        ledcWrite(HEATER_PWM, 4095);
        ledcWrite(FAN_1_PWM, 20); //exhaust
        ledcWrite(FAN_2_PWM, 25); //intake
        
      }

      if(temperatureIncreaseStarted && tempSetPoint > tempActual && elapsed(temperatureIncreaseStart) > 300) { //after 300 seconds of heating, increase the fans
        ledcWrite(HEATER_PWM, 4095);
        ledcWrite(FAN_1_PWM, 30);
        ledcWrite(FAN_2_PWM, 40);
        
      }
      //currently there is no exit condition for heating in performance mode if it does not reach the desired temperature

      if(temperatureIncreaseStarted && tempSetPoint < tempActual) { // stop heating after desired temperature is reached
        temperatureIncreaseStarted = false;
        ledcWrite(FAN_1_PWM, 10); // these just need to set back to baseline airflow
        ledcWrite(FAN_2_PWM, 10);
        ledcWrite(HEATER_PWM, 0);
      }
    }

  }


  // EXTERNAL temperature is TOO WARM
  // DECREASE chamber temp.
  // NOTE: right now this still freaks out when the exact setpoint is breached & this code is not currently in the loop because the air exchange handles it
  void handleTemperatureDecrease() {
    if(habitat_mode == 0) { //QUIET MODE
      
      if (tempActual > (tempSetPoint + 2) && !temperatureDecreaseStarted) {
        Serial.println("temperatureDecreaseStarting");
        ledcWrite(HEATER_PWM, OFF_DUTY);
        ledcWrite(FAN_1_PWM, 9); // prev 10 - but this should just be a mapped value w/ new hardware
        ledcWrite(FAN_2_PWM, 10);
        
        temperatureDecreaseStart = now();
        temperatureDecreaseStarted = true;

      }

      if (temperatureDecreaseStarted) {
        // Serial.println("temperatureDecreaseOngoing");
        int temperatureDecrease = tempActual - tempSetPoint;
      
        if (temperatureDecrease < 2) {
          ledcWrite(FAN_1_PWM, 8);
          ledcWrite(FAN_2_PWM, 9);
          
          temperatureDecreaseStarted = false;
        }

        if (elapsed(temperatureDecreaseStart) > 300) {
          ledcWrite(FAN_1_PWM, 7);
          ledcWrite(FAN_2_PWM, 7);
          //Blynk.logEvent("temperature_decrease_error");
          temperatureDecreaseStarted = false;

        }

      }

    }

    if(!temperatureDecreaseStarted) {}//Blynk.resolveEvent("temperature_decrease_error");}
  }

  void handleHumidityIncrease() { // humidity in chamber is too LOW -> increase

    if (humidityActual < (humiditySetPoint + 1) && !atomizerStarted) {
      // humidityActual < (humiditySetPoint + 1)
      // old if ((humiditySetPoint > humidityActual + 1) && !atomizerStarted) {
      humidityIncreaseStart = now();

      int humidityIncrease = humiditySetPoint - humidityActual;
      float secondsToRun = humidityIncrease / HUMIDITY_PCT_PER_SECOND;

      if(humidityIncrease < 5 && elapsed(humidityIncreaseStart) < 120) {
        ledcWrite(BLOWER_PWM, ON_DUTY);
      }

      else {
        ledcWrite(ATOMIZER_PWM, ON_DUTY);
        //vTaskDelay(pdMS_TO_TICKS(10000)); //hard coded in 10 seconds to build up the "poof"
        //ledcWrite(BLOWER_PWM, ON_DUTY); // poof here
        //ledcWrite(ATOMIZER_PWM, OFF_DUTY);
        atomizerStarted = true;
      }
    }

    if(atomizerStarted && elapsed(humidityIncreaseStart) > 10) {
      ledcWrite(BLOWER_PWM, ON_DUTY); // poof here
      ledcWrite(ATOMIZER_PWM, OFF_DUTY);
    }

    if (atomizerStarted && elapsed(humidityIncreaseStart) > 25) {
      Serial.println("humidityIncreaseOngoing");
      ledcWrite(BLOWER_PWM, OFF_DUTY); // stop poof after 15 additional seconds
    }

    if (elapsed(humidityIncreaseStart) > 30 && habitat_mode == 1) { //the atomizer will run every 30 seconds in performance mode instead of every 60 seconds
      atomizerStarted = false; //reset to enable atomizer to start again
      humidityIncreaseStart = now();
    }

    if (elapsed(humidityIncreaseStart) > 60) {
      atomizerStarted = false; //reset to enable atomizer to start again
      humidityIncreaseStart = now();
    }

  }


  void handleHumidityDecrease() { // humidity in chamber is too HIGH reduce humidity
    if(habitat_mode == 0) { //QUIET MODE
      if (humidityActual > (humiditySetPoint + 1) && !humidityDecreaseStarted) {
        // not fast enough at 10 needs to be higher
        ledcWrite(FAN_1_PWM, 9);
        ledcWrite(FAN_2_PWM, 10);
        
        ledcWrite(BLOWER_PWM, OFF_DUTY);
        Serial.println("humidityDecreaseStart");
        humidityDecreaseStart = now();
        humidityDecreaseStarted = true;
      }

      if (humidityDecreaseStarted) {
        int humidityDecrease = humidityActual - humiditySetPoint;
        Serial.println("humidityDecreaseOngoing");
        
        // make sure this higher speed "burst" can only run for 30s
        // maybe disable atomizer while this is happening?
        if(elapsed(humidityDecreaseStart) > 180) {
          ledcWrite(FAN_1_PWM, 10);
          ledcWrite(FAN_2_PWM, 11);
        }

        if (humidityDecrease < 1) {
          ledcWrite(FAN_1_PWM, 7);
          ledcWrite(FAN_2_PWM, 8);
          humidityDecreaseStarted = false;
        }

        if (elapsed(humidityDecreaseStart) > 300) {
          //Blynk.logEvent("humidity_decrease_error");
          humidityDecreaseStarted = false;
          //add in exit condition that prevents continous tries
        }

        if (elapsed(humidityDecreaseStart) > 7200) {
          //Blynk.logEvent("humidity_decrease_error");
          humidityDecreaseStarted = false;
          //add in exit condition that prevents continous tries
        }
      }

      else {}//Blynk.resolveEvent("humidity_decrease_error");}
    }

    if(habitat_mode == 1) { //PERFORMANCE MODE
      if (humidityActual > (humiditySetPoint + 1) && !humidityDecreaseStarted) {
        ledcWrite(FAN_1_PWM, 20);
        ledcWrite(FAN_2_PWM, 22);
        
        ledcWrite(BLOWER_PWM, OFF_DUTY);
        Serial.println("humidityDecreaseStart");
        humidityDecreaseStart = now();
        humidityDecreaseStarted = true;
      }
      
      if (humidityDecreaseStarted) {
        int humidityDecrease = humidityActual - humiditySetPoint;
        Serial.println("humidityDecreaseOngoing");
        
        // make sure this higher speed "burst" can only run for 30s
        // maybe disable atomizer while this is happening?
        if(elapsed(humidityDecreaseStart) > 180) {
          ledcWrite(FAN_1_PWM, 32);
          ledcWrite(FAN_2_PWM, 35);
        }

        if (humidityDecrease < 1) {
          ledcWrite(FAN_1_PWM, 7);
          ledcWrite(FAN_2_PWM, 8);
          humidityDecreaseStarted = false;
        }
      }
    }
  }


  // make sure this function can't run endlessly
  void handleAirExchange() {
    if(ledcRead(FAN_1_PWM) < 10 || ledcRead(FAN_2_PWM) < 10) {
      //added in simplification for the airflow code 
      // NOTE ADD SOFT CAP HERE
      // a setting of airflow 1 is fans at speed 7 which is the minumum
      Serial.println("Airflow PWM setting: " + String(adj_airflow));
      ledcWrite(FAN_1_PWM, adj_airflow);
      ledcWrite(FAN_2_PWM, adj_airflow + 1); //sets the intake fan to a low setting to keep the terrashroom at positive pressure
    }
  /*
    if(!humidityIncreaseStart && !humidityDecreaseStarted && !temperatureDecreaseStarted && !temperatureIncreaseStarted && !airExchangeStarted && elapsed(airExhangeStart) > 86400/airflow) {
      airExchangeStarted = true;
      airExhangeStart = now();
      ledcWrite(FAN_PWM, 14);
    }

    if(airExchangeStarted && elapsed(airExhangeStart) > 300) {
      airExchangeStarted = false;
    ledcWrite(FAN_PWM, ON_DUTY);
  }
  */
  }

  void sensorCheck() {
    
    if(elapsed(resetHDC1080Start) < 15) {
      //return; this can be for the heating time 
    }

    if(elapsed(resetHDC1080Start) > 3600) {
      hdc1080.heatUp(10);
      resetHDC1080Start = now();
      //return; this can be for the heating time
    }

    if(humidityActual == 99 || tempActual >195) {hdc1080.begin(0x40);} //if not connected try to reconnect
    

    tempActual = hdc1080.readTemperature() * 1.8 + 32; // read in the temperature and the humidity from the
                                              // HDC1080 temperature and humidity sensor
    humidityActual = hdc1080.readHumidity();
  
    round(tempActual);
    round(humidityActual);

    if(tempActual > 195 && first_temp_sensor_fail_notification == false ) {
      temp_sensor_failure_time = now();
      //Blynk.logEvent("temperature_sensor_error");
      first_temp_sensor_fail_notification = true;
    }

    else if(tempActual > 195 && elapsed(temp_sensor_failure_time) > 3600 && second_temp_sensor_fail_notification == false ) {
      //Blynk.logEvent("temperature_sensor_error");
      second_temp_sensor_fail_notification = true;
    }

    else if(tempActual > 195 && elapsed(temp_sensor_failure_time) > 28800 && third_temp_sensor_fail_notification == false ) {
      //Blynk.logEvent("temperature_sensor_error");
      third_temp_sensor_fail_notification = true;
    }

    else if (tempActual < 195) {
      //Blynk.resolveEvent("temperature_sensor_error");
      first_temp_sensor_fail_notification = false;
      second_temp_sensor_fail_notification = false;
      third_temp_sensor_fail_notification = false;
      }



    //Blynk.virtualWrite(V0, tempActual); // reports the read temperature and
                                        // humidity to the blynk app
   // Blynk.virtualWrite(V1, humidityActual);

  }

  

  // function to zero out all habitat control values to allow manual control temporarily
  
  ////////// MAIN EVENT LOOP ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  void loop() {
    //waterlevelreadfreq++;
    sensorCheck();
    safetyCheck();
    if(preventHumidityDamage) {return;}
    //if(waterlevelreadfreq%2 == 1) {WaterLevel();}
    
    if (now() - lastWaterLevelCall >= 2) {
      WaterLevel();
      lastWaterLevelCall = now();
    }
    
    if (manual || !enabled) return;

    //handleAirExchange();

    if (!preventHeatDamage && !airExchangeStarted) {
      handleTemperatureIncrease();
    }

    handleHumidityDecrease();

    if (!preventWaterDamage && !airExchangeStarted) { // checks are in place to make sure we dont do a poof if water level is low
      handleHumidityIncrease();
      
    }

    handleAirExchange();
  }

  void safetyCheck() {

    if (humidityActual > 98) { // humidity damage prevention
      ledcWrite(ATOMIZER_PWM, OFF_DUTY);
     // Blynk.virtualWrite(V19, LOW); // atomizer virtual pin
      ledcWrite(BLOWER_PWM, OFF_DUTY);
      //Blynk.virtualWrite(V21, LOW); // blower virtual pin
      preventHumidityDamage = true;
      // consider adding ramp to this PWM
      ledcWrite(FAN_1_PWM, 100);
      ledcWrite(FAN_2_PWM, 100);
      //ledcWrite(FAN_1_PWM, ON_DUTY);
      //ledcWrite(FAN_2_PWM, ON_DUTY);
    
     // Blynk.virtualWrite(V20, HIGH); // fan virtual pin
      //preventHumidityDamage = true;
      //vTaskDelay(pdMS_TO_TICKS(15000)); we are going to make no other thing function during this time
    }

    if (humidityActual < 90 &&
        preventHumidityDamage) { // if humidity goes back to 95% or lower, it shuts
                                 // off the fans
      preventHumidityDamage = false;
      // this should just go back to airflow NOT off
      ledcWrite(FAN_1_PWM, 8);
      ledcWrite(FAN_2_PWM, 9);
      
    //  Blynk.virtualWrite(V20, LOW); // fan virtual pin
    }

    if (tempActual > 125 || preventHeatDamage) { // over temp damage prevention
      ledcWrite(HEATER_PWM, OFF_DUTY);
     // Blynk.virtualWrite(V17, LOW);
      preventHeatDamage = true;
    }

    if (preventHeatDamage == true && tempActual < 120) {
      preventHeatDamage = false;
    }

    // add in preventing extrodinary run times as well as fire/melt prevention
    // code & abnormal detection from standard water usage counter | add in timer
    // for if "prevent_damage" is high, and if its longer than 1 hour, send PUSH
    // to the user, same thing for temperature (and if the code thinks it has been
    // left in the sun)
    //
  }


  void WaterLevel(void) {

    if(!preventWaterDamage) {

      digitalWrite(WATER_READ_ENABLE, HIGH); //enable sensor
      delay(3);
      //vTaskDelay(pdMS_TO_TICKS(3000));

        if (digitalRead(WATER_LOW)) { // detected water - no notification (what actually is this reading?)
          digitalWrite(WATER_READ_ENABLE, LOW); //disable sensor
         // Blynk.virtualWrite(V8, HIGH); // tells blynk there is water
          preventWaterDamage = false;
          firstWaterLevelNotif = false;
         //Blynk.resolveEvent("low_water");
          hour_no_water = false;
          four_hours_no_water = false;
          day_no_water = false;
        }

        else { // did NOT detect water - send first notification
          digitalWrite(WATER_READ_ENABLE, LOW);
          digitalWrite(UV_LED, LOW);
         // Blynk.virtualWrite(V8, LOW);
          
          if (now() - lastWaterLevelNotif >= 240 || firstWaterLevelNotif == false) {
          //  Blynk.logEvent("low_water");
            firstWaterLevelNotif = true;
            lastWaterLevelNotif = now();
          }
          
          preventWaterDamage = true;
          preventWaterDamageStart = now();
          ledcWrite(ATOMIZER_PWM, OFF_DUTY);
         // Blynk.virtualWrite(V19, LOW); // atomizer virtual pin
        }
          digitalWrite(WATER_READ_ENABLE, LOW);
    }

    else { // EXISTING WATER WARNING

      digitalWrite(WATER_READ_ENABLE, HIGH);
      delay(5);

        if(digitalRead(WATER_LOW)) { // detected water - no notification
          digitalWrite(WATER_READ_ENABLE, LOW);
         // Blynk.virtualWrite(V8, HIGH); // tells blynk there is water
          preventWaterDamage = false;
         // Blynk.resolveEvent("low_water");
          hour_no_water = false;
          four_hours_no_water = false;
          day_no_water = false;
        }

        else if(elapsed(preventWaterDamageStart) > 3600 && firstWaterLevelNotif == true) {
          //Blynk.logEvent("low_water");
          hour_no_water = true;
        }

        else if(elapsed(preventWaterDamageStart) > 14400 && four_hours_no_water == false) {
          //Blynk.logEvent("low_water");
          four_hours_no_water = true;
        }

        else if(elapsed(preventWaterDamageStart) > 86400 && day_no_water == false) {
          //Blynk.logEvent("low_water");
          day_no_water = true;
        }
    }
}

  int tempActual = 0;
  int humidityActual = 0;
  int tempSetPoint;      // Target Temperature of Terrashroom
  int humiditySetPoint;  // Target Humidity of Terrashroom
  int tempTolerance;     // Acceptable Temperature Window of Terrashroom
  int humidityTolerance; // Acceptable Humidity Window of Terrashroom
  int adj_airflow = 7;        // Airflow setting for Terrashroom
};

#endif

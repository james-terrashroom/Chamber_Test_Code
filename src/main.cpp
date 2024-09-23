#include <Arduino.h>
// #include <Adafruit_INA219.h>
#include <ArduinoJson.h>
// #include <Wire.h>
#include "Camera.h"
#include "Electricity.h"
#include "Habitat.h"
#include "LEDArray.h"
#include "Test.h"



const int NUMPIXELS_RING = 24;
const int NUMPIXELS_STRIP = 64; // change to 64 with the new led's


int terrashroom_power = 1;

// PIN DECLARATION VARIABLES
int GPIO0 = 19;
int ring_light_pin = 5;   // this pin (D5) serially controls the ring LED
int strip_light_pin = 14; // this pin (D14) serially controls the strip LED
int pair = 4; //  pair switch (pin D4) reports if the pair switch is pressed  not pressed
int image_Rotation = 0;
int mushroom = 0;
int button;
bool TESTING; // this is high until the testing passes and then it will go low after that forever
int TESTING_ONGOING = 0;
int Count = 0;
String bucketId;
String timelapseID;


LEDArray ring = LEDArray(NUMPIXELS_RING, ring_light_pin);
LEDArray strip = LEDArray(NUMPIXELS_STRIP, strip_light_pin);
Habitat habitat = Habitat();
Camera camera = Camera();
Test test;
Electricity electricity;

TaskHandle_t Task1; // Light animation task on Core 0 declaration

// User Defined Functions
//*************************************************************

void myTimerEvent(void);         // Blynk timer subroutine
void Task1code(void *parameter); // Core 0 code (handles light animation)
void saveToPreferences(const char *key, float value);
void settimelapseURLs(int number);
void updateTest(void);


// SETUP CODE
//*************************************************************
void setup() {
  
  Serial.begin(115200);
  Serial.println("preferences");

  Serial.println("habitat");
  habitat.setup();
  Serial.println("camera");
  camera.setup();
  strip.begin();
  ring.begin();
  strip.clear();
  ring.clear();
  strip.startup();
  ring.startup();

  pinMode(pair,
          INPUT_PULLUP); // Declaring the pair button as an Input GPIO (pulled // high - active low)


  Serial.println("This is the best TESTING firmware ever!!!");

  Serial.println("electricity");
  electricity.setup();
  Serial.println("auth");


  camera.setWifi("Jinbang3-4d", "jinbang123");

  // timeAtomizerHigh = preferences.getFloat("atomizer_time", 0.0); NEED TO DO THIS

  Serial.println("Setup Complete");

  xTaskCreatePinnedToCore( // creation of seperate loop running on core 0 to run
                           // the LEDs
      Task1code,           /* Task function. */
      "Task1",             /* name of task. */
      50000,               /* Stack size of task */
      NULL,                /* parameter of the task */
      1,                   /* priority of the task */
      &Task1,              /* Task handle to keep track of created task */
      0);                  /* pin task to core 0 */
  
  //preferences.putBool("testing", true); // comment this out when done testing code

  //TESTING = preferences.getBool("testing", true); // this is high until the testing passes and then it will go low after that forever
  TESTING = true;
  if(TESTING) {
    camera.setFlashEnabled(1); camera.setEnabled(1);
  }
  delay(500);
  ring.clear();
  strip.clear();
  delay(500);
}

void loop() { // this loop runs on core 1


  if(TESTING && test.test_Number == 1) {
    ring.setBrightness(255);
    ring.startup();
    ring.fillWhite();
  }

  else if(TESTING && test.test_Number == 2) {
    ring.clear();
    strip.setBrightness(255);
    strip.startup();
    strip.fillWhite();
  }

  else if(TESTING) {
    strip.clear();
    strip.setOn(false);
    ring.clear();
    ring.setOn(false);
  }

  vTaskDelay(pdMS_TO_TICKS(25));
}

void Task1code(void *parameter) { // runs on core 0
  for (;;) {
   
    vTaskDelay(pdMS_TO_TICKS(10));

 // RunTime(); //logs run time of the fans (to determine filter life)

    
    camera.loop();

    if(!TESTING) {
         // runs timer to execute "myTimerEvent" every second
    electricity.loop();
    }
    
    if(TESTING) {
      test.run();
      test.loop();
      updateTest();
      test.took_picture = camera.cameraTestSuccessful;
    }
  }
}

void updateTest(void) {
  if(test.test_Number > 0) {} //Blynk.virtualWrite(V61, "测试进行中"); Blynk.virtualWrite(V123, 255); Blynk.setProperty(V59, "color", "#eeFF00");}

  if (test.top_Light_Status == 2) {} //Blynk.setProperty(V110, "color", "#7AFB63");}
  else if (test.top_Light_Status == 0) {} //Blynk.setProperty(V110, "color", "#eeFF00");}
  else if (test.top_Light_Status == 1) {} //Blynk.setProperty(V110, "color", "#FF0011");}

  if (test.bottom_Light_Status == 2) {} //Blynk.setProperty(V111, "color", "#7AFB63");}
  else if (test.bottom_Light_Status == 0) {} //Blynk.setProperty(V111, "color", "#eeFF00");}
  else if (test.bottom_Light_Status == 1) {} //Blynk.setProperty(V111, "color", "#FF0011");}

  if (test.water_Sensor_Status == 2) {} //Blynk.setProperty(V112, "color", "#7AFB63");}
  else if (test.water_Sensor_Status == 0) {} //Blynk.setProperty(V112, "color", "#eeFF00");}
  else if (test.water_Sensor_Status == 1) {} //Blynk.setProperty(V112, "color", "#FF0011");}

  if (test.fan_1_Status == 2) {} //Blynk.setProperty(V113, "color", "#7AFB63");}
  else if (test.fan_1_Status == 0) {} //Blynk.setProperty(V113, "color", "#eeFF00");}
  else if (test.fan_1_Status == 1) {} // Blynk.setProperty(V113, "color", "#FF0011");}

  if (test.fan_2_Status == 2) {} //Blynk.setProperty(V114, "color", "#7AFB63");}
  else if (test.fan_2_Status == 0) {} //Blynk.setProperty(V114, "color", "#eeFF00");}
  else if (test.fan_2_Status == 1) {} //Blynk.setProperty(V114, "color", "#FF0011");}

  if (test.blower_Status == 2) {} // Blynk.setProperty(V115, "color", "#7AFB63");}
  else if (test.blower_Status == 0) {} //Blynk.setProperty(V115, "color", "#eeFF00");}
  else if (test.blower_Status == 1) {} // Blynk.setProperty(V115, "color", "#FF0011");}

  if (test.camera_Status == 2) {} // Blynk.setProperty(V116, "color", "#7AFB63");}
  else if (test.camera_Status == 0) {} // Blynk.setProperty(V116, "color", "#eeFF00");}
  else if (test.camera_Status == 1) {} // Blynk.setProperty(V116, "color", "#FF0011");}

  if (test.atomizer_Status == 2) {} // Blynk.setProperty(V117, "color", "#7AFB63");}
  else if (test.atomizer_Status == 0) {} //Blynk.setProperty(V117, "color", "#eeFF00");}
  else if (test.atomizer_Status == 1) {} //Blynk.setProperty(V117, "color", "#FF0011");}

  if (test.uv_Status == 2) {} // Blynk.setProperty(V118, "color", "#7AFB63");}
  else if (test.uv_Status == 0) {} // Blynk.setProperty(V118, "color", "#eeFF00");}
  else if (test.uv_Status == 1) {} // Blynk.setProperty(V118, "color", "#FF0011");}

  if (test.hdc1080_Status == 2) {} // Blynk.setProperty(V119, "color", "#7AFB63");}
  else if (test.hdc1080_Status == 0) {} //Blynk.setProperty(V119, "color", "#eeFF00");}
  else if (test.hdc1080_Status == 1) {} //Blynk.setProperty(V119, "color", "#FF0011");}

  if (test.heater_Status == 2) {} //Blynk.setProperty(V120, "color", "#7AFB63");}
  else if (test.heater_Status == 0) {} //Blynk.setProperty(V120, "color", "#eeFF00");}
  else if (test.heater_Status == 1) {} //Blynk.setProperty(V120, "color", "#FF0011");}

  if(test.voltag_Status == 2) {} //Blynk.virtualWrite(V122, "color", "7AFB63");}
  else if(test.voltag_Status == 0) {} // {Blynk.virtualWrite(V122, "color", "#eeFF00");}
  else if(test.voltag_Status == 1) {} //Blynk.virtualWrite(V122, "color", "#FF0011");}


  if(test.hdc1080_Status == 2 && test.atomizer_Status == 2 && test.camera_Status == 2 && test.fan_1_Status == 2 && test.fan_2_Status == 2 && test.top_Light_Status == 2 && test.bottom_Light_Status == 2 && test.water_Sensor_Status == 2 && test.uv_Status == 2 && test.heater_Status == 2 && test.voltag_Status == 2) {
    TESTING = false;
    Serial.println("TESTING PASSED");
    delay(1000);
    //Blynk.virtualWrite(V59, 255); Blynk.setProperty(V59, "color", "#7AFB63");
    //Blynk.virtualWrite(V61, "考试通过了!");
    ESP.restart();
  }
  else if(test.test_Number > 12) {} //{Blynk.virtualWrite(V61, "测试失败!"); Blynk.virtualWrite(V123, 255); Blynk.setProperty(V59, "color", "#FF0011");}
}




void myTimerEvent() {
  Count++;
  habitat.loop();
  electricity.updateApp();
  
  //if(!TESTING) {Blynk.setProperty(V46, "isHidden", true);} // hide the testing button

  if(Count % 10 == 0) {
    settimelapseURLs(random(0,10000));
  }

}


void settimelapseURLs(int number) {
  Serial.println("number: " + String(number));
 
  //Blynk.virtualWrite(V55, 1);
  
  vTaskDelay(pdMS_TO_TICKS(100));
  String urlStringBucketId = camera.getBucketId();
 // Blynk.setProperty(V55, "urls", "https://app.terrashroom.org/timelapse/" + urlStringBucketId + "/latest.jpg?t=" + String(number), "https://terrashroom-public.s3.us-west-2.amazonaws.com/app_images/images.png");

  //Blynk.virtualWrite(V55, 0);

 //Blynk.setProperty(V5, "url", "https://app.terrashroom.org/timelapse/" + urlStringBucketId); //we need to set V5 to the URL of the timelapse
  //Blynk.virtualWrite(V56, "https://app.terrashroom.org/timelapse/" + urlStringBucketId); //we need to set V5 to the URL of the timelapse
  //Blynk.syncVirtual(V55, V5);
}


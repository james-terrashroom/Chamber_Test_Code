#ifndef CAMERA_H
#define CAMERA_H


#include <ArduinoJson.h>

const int ESPCAM_ENABLE = 26;
const int CAMERA_UPDATE_WINDOW = 60000; // 60 seconds
const int CAMERA_OFF_DELAY = 30000;     // 30 seconds


const String NULL_BUCKET_ID = "NULL_BUCKET_ID";


class Camera {
public:

  void setup() {

    pinMode(ESPCAM_ENABLE, OUTPUT);
    digitalWrite(ESPCAM_ENABLE, LOW);
    //we need to add in a delay here to make sure the camera is truly off before we turn it on
    delay(1000);
    setCameraOn(true);
    //cameraOffAfter(CAMERA_UPDATE_WINDOW);
    
  }

  void setEnabled(bool value) {
    if (enabled == value) return;

    enabled = value;
    if (enabled) {
      Serial.println("Enabling camera...");
      cancelCameraOff();
      setCameraOn(true);
      setTimelapseEnabled(true);
      Serial.println("Camera enabled.");
    } else {
      Serial.println("Disabling camera...");
      setTimelapseEnabled(false);
      cameraOffAfter(CAMERA_OFF_DELAY);
    }
  }

  void setCameraOn(bool value) {
    if (cameraOn == value) return;

    cameraOn = value;
    if (cameraOn) {
      digitalWrite(ESPCAM_ENABLE, HIGH);
      delay(500);
      Serial2.begin(115200);
      Serial.println("Turning camera on...");
      while (!Serial2) {
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    } else {
      Serial.println("Turning camera off...");
      Serial2.end();
      digitalWrite(ESPCAM_ENABLE, LOW);
    }
  }

  void restart() {
    if (cameraOn) {
      Serial.println("Restarting camera...");
      setCameraOn(false);
      vTaskDelay(pdMS_TO_TICKS(1000));
      setCameraOn(true);
    }
  }


  // main event loop for CAMERA on CHAMBER main board
  void loop() {
    
    if (cameraOffAt && cameraOffAt < millis()) {
      cameraOffAt = 0;
      setCameraOn(false);
    }

    // WAIT for incoming serial from camera
    // could be a problem since this assumes the main board will boot before the camera
    // power toggling should only be used to make sure main-board boots before the camera
    if (Serial2) {
      now = millis();
      String inputString = readStringUntil('\n');
      if (inputString == "") return;
      //Serial.print("<= ");
      //Serial.println(inputString);
      if(inputString.length() < 255) {}//Blynk.virtualWrite(V53, inputString);}

      // turn received serial datastring into json obj
      if (inputString.startsWith("{")) {
        StaticJsonDocument<1024> cmd;
        DeserializationError err = deserializeJson(cmd, inputString);
        if (err) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(err.f_str());
          return;
        } else {
          // parse json and decide whether or not camera needs to be re-flashed with newer firmware
          handleCommand(cmd);
        }
      }

      // NOTE ~ FUTURE: replace this with a distinct string
      if (inputString.startsWith("Not connected")) {
        sendWifi(); //if the camera is not connected to wifi, send the wifi credentials again
      }

      if(inputString.startsWith("Camera capture succ")) {
        cameraTestSuccessful = 1;
        //this is for the testing code, if the camera is able to take an image then it passes the testing
       // Serial.println("Camera capture successful and took_picture set to:");
      }
    }

    if((millis() - now) > 10000) {
      Serial.println("Camera not responding");
      restart();
    }
  }

  void setWifi(String ssid, String password) {
    this->ssid = ssid;
    this->password = password;
    sendWifi();
  }

  void setTimelapseEnabled(bool enabled) {
    Serial.println("Setting timelapse enabled: " + String(enabled));
    timedlapseEnabled = enabled;
    sendTimelapseSettings();
  }

  void setTimelapseInterval(int interval) {
    Serial.println("Setting timelapse interval: " + String(interval));
    timelapseInterval = interval;
    sendTimelapseSettings();
  }

  void setFlashEnabled(bool enabled) {
    Serial.println("Setting flash enabled: " + String(enabled));
    flashEnabled = enabled;
    sendTimelapseSettings();
  }

  void setBucketId(String id) {
    Serial.println("Setting bucket id: " + id);
    if (id == NULL_BUCKET_ID) {
     
      bucketId = "111111111";
    } else {
      bucketId = id;
    }
    sendTimelapseSettings();
  }

  String getBucketId() {
    return bucketId;
  }


  int cameraTestSuccessful = 0;
  String timelapseID = "";
  int timelapseCount = 0;
  

private:
  String ssid = "";
  String password = "";
  String received = "";
  bool enabled = false;
  bool cameraOn = false;
  bool timedlapseEnabled = false;
  bool flashEnabled = false;
  uint16_t timelapseInterval = 0;
  String bucketId = "";
  unsigned long cameraOffAt = 0;
  unsigned long now = 0;

  void cameraOffAfter(uint16_t delay) {
    cameraOffAt = millis() + delay;
  }

  void cancelCameraOff() {
    cameraOffAt = 0;
  }

  // NOTE ~ OTA: this is where the chamber decides if the camera should update or not
  void handleCommand(JsonDocument &cmd) {
    if (cmd["status"] == "ready") {
      sendWifi();
      sendTimelapseSettings();
      //Blynk.virtualWrite(V_CAMERA_VERSION, cmd["version"].as<String>());

     // if (cmd["version"] != CAMERA_VERSION && CAMERA_HASH != "") {
        // if mismatch here then force update
     //   queueFirmwareUpdate(CAMERA_VERSION, CAMERA_HASH);
     // }
    } else if (cmd["restart"]) {
      restart();
    }
    if(cmd["timelapse"]) {
    //  Blynk.virtualWrite(V54, cmd["timelapse"].as<String>());
      // this comes FROM CAMERA since it can talk to AWS
      timelapseID = cmd["timelapse"].as<String>();
    }
    if(cmd["timelapseCount"]) {
    //  Blynk.virtualWrite(V57, cmd["timelapseCount"].as<int>());
      timelapseCount = cmd["timelapseCount"].as<int>();
    }
  }

  void sendWifi() {
    if (Serial2 && ssid != "") {
      Serial.println("Sending wifi credentials");
      StaticJsonDocument<200> doc;
      doc["ssid"] = ssid;
      doc["password"] = password;

      Serial2.println();
      serializeJson(doc, Serial2);
      Serial2.println();
    }
  }

/*
  void sendtimelapsestatus() {
    if (Serial2 && Blynk.connected()) {
      Serial.println("Sending timelapse status");
      StaticJsonDocument<200> doc;
      BLYNK_WRITE(V54);
      BLYNK_WRITE(V57);
      
      doc["timelapseID"] = timelapseID;
      doc["timelapseCount"] = timelapseCount;

      Serial2.println();
      serializeJson(doc, Serial2);
      Serial2.println();
    }
  }
  */

  void sendTimelapseSettings() {
    
    if (Serial2) {
      //sendtimelapsestatus();
      //delay(1000);
      Serial.println("Sending timelapse settings");
      StaticJsonDocument<200> doc;
      doc["timelapse"] = timedlapseEnabled;
      doc["interval"] = timelapseInterval;
      doc["flash"] = flashEnabled;
      doc["bucket_id"] = bucketId;

      Serial2.println();
      serializeJson(doc, Serial2);
      Serial2.println();
    }
  }

  // Does the CAMERA request this or does it WAIT for the chamber to SEND this?
  void queueFirmwareUpdate(const String &version, const String &hash) {
    if (Serial2) {
      Serial.println("Queuing firmware update");
      StaticJsonDocument<200> doc;
      // construct json object with current version and hash
      JsonObject ota = doc.createNestedObject("ota");
      ota["version"] = version;
      ota["hash"] = hash;

      Serial2.println();
      serializeJson(doc, Serial2);
      Serial2.println();
    }
  }

  String readStringUntil(char terminator) {
    // Non-blocking read from Serial.
    while (Serial2.available() > 0) {
      char c = Serial2.read();
      if (c == terminator) {
        String result = received;
        received = "";
        return result;
      }
      received += c;
    }
    return "";
  }
};

#endif

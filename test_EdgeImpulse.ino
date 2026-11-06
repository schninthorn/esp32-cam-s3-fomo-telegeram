/**
 * Collect images for Edge Impulse image
 * classification / object detection
 *
 * BE SURE TO SET "TOOLS > CORE DEBUG LEVEL = INFO"
 * to turn on debug messages
 */

// if you define WIFI_SSID and WIFI_PASS before importing the library, 
// you can call connect() instead of connect(ssid, pass)
//
// If you set HOSTNAME and your router supports mDNS, you can access
// the camera at http://{HOSTNAME}.local

//#define WIFI_SSID "Tian"
//#define WIFI_PASS "91992189327"


#define WIFI_SSID "ROSLabStudio"
#define WIFI_PASS "w973hPPaATh6MSTi"
#define HOSTNAME "esp32cam"


#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/wifi/sta.h>
#include <eloquent_esp32cam/viz/image_collection.h>
#include "esp_camera.h"

using eloq::camera;
using eloq::wifi;
using eloq::viz::collectionServer;


void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("___IMAGE COLLECTION SERVER___");

    // camera settings
    // replace with your own model!
    camera.pinout.wroom_s3();
    //camera.brownout.disable();
    // Edge Impulse models work on square images
    // face resolution is 240x240
    //camera.resolution.face();
    //camera.quality.high();
    camera.resolution.qvga();
    camera.quality.best();

    // init camera
    while (!camera.begin().isOk())
        Serial.println(camera.exception.toString());

   sensor_t *s = esp_camera_sensor_get();

    if (s != nullptr) {
        s->set_vflip(s, 1);    // กลับภาพแนวตั้ง
        s->set_hmirror(s, 1);  // กลับภาพแนวนอน
    }else {
        Serial.println("ERROR: Cannot get camera sensor");
    }

    // connect to WiFi
    while (!wifi.connect().isOk())
      Serial.println(wifi.exception.toString());

    // init face detection http server
    while (!collectionServer.begin().isOk())
        Serial.println(collectionServer.exception.toString());

    Serial.println("WiFi OK");
    Serial.println("Camera OK");
    Serial.println("Image Collection Server OK");
    Serial.println(collectionServer.address());
}


/*
void setup() {
    delay(3000);
    Serial.begin(115200);

    Serial.println("___IMAGE COLLECTION SERVER___");

    Serial.println("STEP 1");
    camera.pinout.wroom_s3();

    Serial.println("STEP 2");
    //camera.brownout.disable();

    Serial.println("STEP 3");
    camera.resolution.qvga();

    Serial.println("STEP 4");
    camera.quality.best();

    Serial.println("STEP 5 - BEFORE CAMERA BEGIN");

    while (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
    }

    Serial.println("STEP 6 - CAMERA OK");
}
*/

void loop() {
    // server runs in a separate thread, no need to do anything here
}
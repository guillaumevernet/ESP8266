/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <Servo.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "";
int pos;
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "";
char pass[] = "";


Servo myservo;


BLYNK_WRITE(V1)
{
  int pinValue = param.asInt(); 
  if(pinValue == 1){
    for (int i = 0; i<2; i++){
    for (pos = 90; pos <= 115; pos += 1) {
    // in steps of 1 degree
      myservo.write(pos);           
      delay(5);                      
    }
    for (pos = 115; pos >= 90; pos -= 1) {
      myservo.write(pos);             
      delay(5);                       
    }
    delay(200);
    }
  }else{
        for (pos = 90; pos <= 115; pos += 1) {
    // in steps of 1 degree
      myservo.write(pos);              
      delay(5);                       
    }
    for (pos = 115; pos >= 90; pos -= 1) { 
      myservo.write(pos);              
      delay(5);                       
    }
    Serial.println("notcool");
    digitalWrite(2, LOW);
    delay(100);
  }
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  myservo.attach(4);
  Blynk.begin(auth, ssid, pass);
}

void loop()
{
  Blynk.run();
}

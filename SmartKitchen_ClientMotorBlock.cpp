//This code is a client for the motor block and connects to Smart Kitchen server. It reads the value
//from MacIoT board and changes motor PWM based on the switches input from MacIoT.

//Include libraries
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ESPAsyncWebServer.h"
#include <Wire.h>
#include <ESP32Encoder.h>
ESP32Encoder encoder;

#define SLEEP 16 // used to idle the motor and make sure the drive is working, no need to change
#define PMODE 27 // used to set the drive mode for the motor controller, leave as defined below
#define EN_PWM 32 // PWM to set motor "torque/speed, etc..."
#define DIR 33 

int count=0;
String PWM_speed;

const char* ssid = "Smart Kitchen";
const char* password = "MyKitchen2023";
AsyncWebServer server(80);

//Server for retrieving PWM speed
const char* serverNameTemp = "http://192.168.4.1/motor";

unsigned long previousMillis = 0;
const long interval = 50;

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  // Your IP address with path or Domain name with URL path
  http.begin(serverName);
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  String payload = "--";
  if (httpResponseCode>0) {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
  return payload;
}

void setup() {
  //Initalize serial monitors and connect to custom access point
  Wire.begin();
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  }
    server.begin();

    pinMode(SLEEP, OUTPUT);
    pinMode(PMODE, OUTPUT);
    pinMode(DIR, OUTPUT);
    digitalWrite(SLEEP, HIGH);
    digitalWrite(PMODE, LOW);
    digitalWrite(DIR, LOW);
    //channel 0, 10Khz, 8 bit
    /* Setup the motor driver PWM to 10khz and 8 bit resolution
    * Note: ESP32 Arduino libraries annoyingly call the PWM timer module ledc,
    as if its the only thing people will be driving.
    */
    ledcSetup(0, 10000, 8);
    ledcAttachPin(EN_PWM, 0);
    ledcWrite(0, 127);

    // use pin 25 and 26 for the first encoder
    encoder.attachHalfQuad(25, 26);
    // set starting count value after attaching
    encoder.setCount(15);
    Serial.println("Encoder Start = " + String((int32_t)encoder.getCount()));
}

void loop(){
    
    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis >= interval) {

    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED ){
        PWM_speed = httpGETRequest(serverNameTemp);
        previousMillis = currentMillis;
    }
    ledcWrite(0, PWM_speed.toInt());
    delay(500);
  }
}



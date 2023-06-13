//This code is for the client sensor block. The sensor block reads the temperature and humidity values and 
//publishes it to the server via HTTP for the MacIoT board to retrieve and publish to the webpage.
//Angle is calculated using IMU sensor and is published to server for MacIoT and checks if fridge door is open or closed.

//Include Libraries
#include <WiFi.h>
#include <HTTPClient.h>
#include "ESPAsyncWebServer.h"
#include <Arduino.h>
#include <Arduino_LSM9DS1.h>
#include <Wire.h>
#include <Si7020.h>

//Define temperature/humidity sensor
Si7020 tempRH = Si7020();

//Set the range of the accelerometer,
//which in turns will affect the sensitivity of your measurement
#define LSM9DS1_ACCELRANGE_2G (0b00 << 3)
#define LSM9DS1_ACCELRANGE_16G (0b01 << 3)
#define LSM9DS1_ACCELRANGE_4G (0b10 << 3)
#define LSM9DS1_ACCELRANGE_8G (0b11 << 3) // equal to 0b 0001-1000


//Connect to custom access point
const char* ssid = "Smart Kitchen";
const char* password = "MyKitchen2023";
char buffer[100]; //Variable where angle is going to be stored

AsyncWebServer server(80);
const char* Status = "";
unsigned long previousMillis = 0;
const long interval = 50;

float angle;
float ay,ax,az;
float fridgeTemp, fridgeHumidity;

void setup() {

//Initialize sensors and serial monitor
    Wire.begin();
    tempRH.begin();
    IMU.begin();
    Serial.begin(9600);

    //Define staticIP address for server
    IPAddress staticIP(192, 168, 4, 2);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.config(staticIP, gateway, subnet);
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
}

void loop(){
    // Obtain temperature and humidity data from sensor
    fridgeTemp = tempRH.getTemp();
    fridgeHumidity = tempRH.getRH();
    if (IMU.accelerationAvailable()){
        IMU.readAcceleration(ax,ay,az);
        //Calculate inclination angle based on accelerometer data
        angle = (atan2(ax,az)*180) / PI;

        //Check angle status and if greater than 0.5 degrees, door status updates to open
        if (angle > 0.5){
            Status = "Open";
        }

        else{
            Status = "Closed";
        }
    }

    else{
        Serial.println("IMU Not Available");
    }
    
    //Publish fridge temperature, humidity and angle data to server via HTTP
    server.on("/angle", HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/plain", Status);});
    server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/plain", String(fridgeTemp).c_str());});
    server.on("/RH", HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/plain", String(fridgeHumidity).c_str());});
    delay(1000);
}
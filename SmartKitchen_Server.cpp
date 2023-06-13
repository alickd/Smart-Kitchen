//This code is for the server for the Smart Kitchen project. It reads the data via HTTP from the sensor block 
//and publishes it to the webpage. The flame sensor and LED are connected to the board and reads the values.
//Motor input for fan is sent when stove is on and can be changed using PWM via the switches on the board.

// Include libraries 
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <MCP23017.h> //IO serial expander library
#include <Si7020.h> //Temperature sensor library
#include <AsyncAPDS9306.h> // Light sensor library
#include <Wire.h> //I2C Communication

//Defining the I2C Address of the MCP
#define MCP23017_ADDR 0x20

MCP23017 mcp = MCP23017(MCP23017_ADDR);
Si7020 temp_humidity_sensor;
AsyncAPDS9306 light_sensor;

const char* network = "Smart Kitchen";
const char* password = "MyKitchen2023";

//Servers to retrieve data from sensor block
const char* serverNameAngle = "http://192.168.4.2/angle";
const char* serverNameTemp = "http://192.168.4.2/temp";
const char* serverNameRH = "http://192.168.4.2/RH";

AsyncWebServer server(80);
unsigned long previousMillis = 0;
const long interval = 1000;
const int signalPin = 36;
const int lightPin = 27;

int flameValue;
String angle;
String door;
String SBtemp, SBhumidity;
String StoveStat, LightStat, DoorStat, FanStat;
int motion;
float temp, humidity;
float input;

//Create simple webpage to display sensor data
String SendHTML(float Temperaturestat, float Humiditystat, String StoveStatus, String FanStatus, String LightStatus, String DoorStatus, String fridgeTemp, String fridgeRH) {
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<meta http-equiv=\"refresh\" content=\"2\">\n";
  ptr += "<title>My Kitchen</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>My Kitchen</h1>\n";
  
  //Change colour of text when flame is detected and print it in red
  String stoveColor = "grey";
  if (StoveStatus == "ON") {
    stoveColor = "red";
 }

  //Change colour of text when light is turned on and print it in yellow
  String lightColor = "grey";
  if (LightStatus == "ON") {
    lightColor = "orange";
 }

 //Change colour of text when door is opened and print it in black
  String doorColor = "grey";
  if (DoorStatus == "OPEN") {
    doorColor = "black";
 }

  //Display Smart Kitchen System values on webpage
  ptr += "<p>Temperature: ";
  ptr += (float)Temperaturestat;
  ptr += " C</p>";
  ptr += "<p>Humidity: ";
  ptr += (int)Humiditystat;
  ptr += "%</p>";

  ptr += "<p>Stove: <span style=\"color:";
  ptr += stoveColor;
  ptr += "\">"; 
  ptr += StoveStatus;
  ptr += "</span></p>";

  ptr += "<p>Fan Speed: <span style=\"color:";
  ptr += "\">"; 
  ptr += FanStatus;
  ptr += "</span></p>";

  ptr += "<p>Lighting: <span style=\"color:";
  ptr += lightColor;
  ptr += "\">";    
  ptr += LightStatus;
  ptr += "</span></p>";

  ptr += "<p>Fridge Temperature: ";
  ptr += fridgeTemp;
  ptr += " C</p>";
  ptr += "<p>Fridge Humidity: ";
  ptr += fridgeRH;
  ptr += "%</p>";

  ptr += "<p>Fridge Door: <span style=\"color:";
  ptr += doorColor;
  ptr += "\">";    
  ptr += DoorStatus;
  ptr += "</span></p>";

  ptr += "</p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

// HTTP function to retrieve data 
String httpGETRequest(const char* serverName) {
    HTTPClient http;
    // Your IP address with path or Domain name with URL path
    http.begin(serverName);
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    String payload = "--";
    if (httpResponseCode>0) {
        //Serial.print("HTTP Response code: ");
        //Serial.println(httpResponseCode);
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

void setup()
{
  //Initalize serial monitors and sensors
  Serial.begin(9600);
  temp_humidity_sensor.begin();
  light_sensor.startLuminosityMeasurement();
  // Assign LED as Output
  pinMode(lightPin, OUTPUT);

  // Join the I2C bus
  Wire.begin();
  //Initialize MCP
  mcp.init();

  //Port A as Output (LEDS) || Port B as Input (Switches & Buttons)
  mcp.portMode(MCP23017Port::A, 0); //0 = Pin is configured as output (LEDs)
  mcp.portMode(MCP23017Port::B, 0b11111111); // 1 = Pin is configured as input (switches)

  // Create custom access point 
  Serial.print("Setting AP (Access Point)...");
  WiFi.softAP(network, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //Publish data to server 
  server.on("/motor", HTTP_GET, [](AsyncWebServerRequest *request){String responseHtml = String(input); request->send(200, "text/html", responseHtml);});
  server.on("/MyKitchen", HTTP_GET, [](AsyncWebServerRequest *request){String html = SendHTML(temp, humidity, StoveStat, FanStat, LightStat, DoorStat, SBtemp, SBhumidity);request->send(200, "text/html", html);});
  server.begin();
}

void loop()
{
  //Define variables
  int sw1, sw2, sw3,sw4, btn1, btn2;
 
  temp = temp_humidity_sensor.readTemp();
  humidity = temp_humidity_sensor.getRH();
  AsyncAPDS9306Data data = light_sensor.syncLuminosityMeasurement();
  float lux = data.calculateLux();

  //Set first 4 LEDs off
  mcp.digitalWrite(0,0);
  mcp.digitalWrite(1,0);
  mcp.digitalWrite(2,0);
  mcp.digitalWrite(3,0);

  flameValue = analogRead(signalPin);

  // Check status of fridge 
  sw1=mcp.digitalRead(8); //Check status of button GPB4
    if (sw1==1){
      mcp.digitalWrite(4,1); //Turn on GPA4
      btn1 = mcp.digitalRead(12); // When button is pressed, display fridge temperature
      btn2 = mcp.digitalRead(13); // When button is pressed, display fridge humidity

      if (btn1==0){
        Serial.print("\n");
        Serial.println("Fridge temperature is "); //Turn off GPA4 and print temperature of fridge to serial monitor when button is pressed
        Serial.print(temp);
        Serial.print(" C");
      }
        if (btn2==0){
          Serial.print("\n");
          Serial.println("Fridge humidity is "); //Turn off GPA5 and print humidity of fridge to serial monitor when button is pressed
          Serial.print(humidity);
          Serial.print(" %");
        }
    } 
      else{
        mcp.digitalWrite(4,0); //Turn off GPA4
      }

  // Check status of freezer
  sw2=mcp.digitalRead(9); //Check status of button GPB5
    if (sw2==1){
      mcp.digitalWrite(5,1); //Turn on GPA5
      btn1 = mcp.digitalRead(12); // When button is pressed, display freezer temperature

      if (btn1==0){
        Serial.print("\n");
        Serial.println("Freezer temperature is "); //Turn off GPA5 and print temperature of freezer to serial monitor when button is pressed
        Serial.print(temp);
        Serial.print(" C");
      }
  
    }
      else{
        mcp.digitalWrite(5,0); //Turn off GPA5
      }

     if (flameValue < 4000){
        Serial.println("STOVE is ON");
        StoveStat = "ON";
        mcp.digitalWrite(6,1); //Turn off GPA6
        input = 60;
        FanStat = "LOW";
        sw3=mcp.digitalRead(10); //Check status of button GPB5
          if (sw3==1){
            input = 90;
            FanStat = "MED";
          }
        sw4=mcp.digitalRead(11); //Check status of button GPB5
          if (sw4==1){
            input = 120;
            FanStat = "HIGH";
          }
     }

    else{
      mcp.digitalWrite(6,0); //Turn off GPA6
      StoveStat = "OFF";
      input = 0;
      FanStat = "OFF";
    }

    if (lux < 250){
        digitalWrite(27, HIGH);
        LightStat = "ON";
    }

    else{
      digitalWrite(27,LOW);
      LightStat = "OFF";
    }

    // Retrieve fridge door status via HTTP
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval) {
      door = httpGETRequest(serverNameAngle);
      SBtemp = httpGETRequest(serverNameTemp);
      SBhumidity = httpGETRequest(serverNameRH);
      previousMillis = currentMillis;
      if (door == "Open"){
          DoorStat = "OPEN";
      }
      else{
        DoorStat = "CLOSED";
    }
    }

}


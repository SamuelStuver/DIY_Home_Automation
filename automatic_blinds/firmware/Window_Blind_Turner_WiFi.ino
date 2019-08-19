#include <Arduino.h>
#include <Stepper.h>
#include <string.h>
#include <WiFi.h>
#include <WiFiConfig.h>
#include <HTTPClient.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define MAX_REVOLUTIONS 8.5
#define JOYSTICK_MAX_ANALOG 4095
#define SERVICE_UUID        "e6d0cf52-0695-43d1-bab8-5fcfe298a94a"
#define CHARACTERISTIC_UUID "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c"


void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step);
void moveMotorPercentage(Stepper stepper_obj, float percent, int *current_step);
void stopMotor(Stepper stepper_obj);
void ConnectToWiFi();
void setupOTA();
void getRequest();

const char* host = "192.168.0.24";

uint32_t value = 0;

const int SW_pin = 13; // digital pin connected to switch output
const int X_pin = 32; // analog pin connected to X output
const int Y_pin = 33; // analog pin connected to Y output
const int step_pins[4] = {25, 26, 27,14}; // Stepper motor driver pins
const int enA[2] = {34, 25}; //Stepper motor enable pins

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
const int MAX_STEPS = MAX_REVOLUTIONS * stepsPerRevolution;
// for your motor
int speed = 60;
float direction = 0.0;
int current_step = 0;

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, step_pins[0], step_pins[1], step_pins[2], step_pins[3]);

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;


void setup() {

  Serial.begin(9600);

  // set pins for analog stick
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  
  // set the motor speed:
  myStepper.setSpeed(speed);

  ConnectToWiFi();
  setupOTA();

  

}


void loop() {

  ArduinoOTA.handle();
  getRequest();

}


void moveMotorPercentage(Stepper stepper_obj, float percent, int *current_step)
{
  /****************************************
   * NEED TO CUT POWER USING ENABLE INPUTS ON L298 AFTER MOVING MOTOR, THEN RE-ENABLE POWER BEFORE MOVING
   ***************************************/

  int n_steps = (MAX_STEPS * percent) / 100;
  int sign;
  if (n_steps < 0) {
    sign = -1;
    n_steps = -n_steps;
  } else {
    sign = 1;
  }
  while (n_steps-- > 0 && *current_step <= MAX_STEPS && *current_step >= 0) {
    stepper_obj.step(sign);
    *current_step += sign;
  }
}

void moveMotorToPercentage(Stepper stepper_obj, float ending_percent, int *current_step)
{
  /****************************************
   * NEED TO CUT POWER USING ENABLE INPUTS ON L298 AFTER MOVING MOTOR, THEN RE-ENABLE POWER BEFORE MOVING
   ***************************************/
  int end_step = (MAX_STEPS * ending_percent) / 100;
  int diff_steps = end_step - *current_step;
  int sign = 1;
  if (diff_steps < 0) {
    sign = -1;
    diff_steps = -diff_steps;
  }
  while (diff_steps-- > 0 && *current_step <= MAX_STEPS && *current_step >= 0) {
    stepper_obj.step(sign);
    *current_step += sign;
  }
}

void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step)
{

  int n_steps = 10;
  while (n_steps-- > 0 && *current_step <= MAX_STEPS && *current_step >= 0) {
    stepper_obj.step(sign);
    *current_step += sign;
  }

}

void stopMotor(Stepper stepper_obj)
{
  stepper_obj.step(0);
}

void setupOTA()
{
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void ConnectToWiFi()
{
  // We start by connecting to a WiFi network
    WiFi.begin(SSID, WiFiPassword);

    Serial.println();
    Serial.println();
    Serial.print("Waiting for WiFi... ");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    delay(500);
}


int server_value = 0;

void getRequest()
{
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    //http.begin("http://jsonplaceholder.typicode.com/comments?id=10"); //Specify the URL
    
    http.begin("http://192.168.0.24:8080");
    
    int httpCode = http.GET();                                        //Make the request
    Serial.println(httpCode);
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
 
  delay(5000);
}
#include <Arduino.h>
#include <Stepper.h>
#include <string.h>
#include <WiFi.h>
#include <WiFiConfig.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define MAX_REVOLUTIONS 8.5
#define JOYSTICK_MAX_ANALOG 4095
#define SERVICE_UUID        "e6d0cf52-0695-43d1-bab8-5fcfe298a94a"
#define CHARACTERISTIC_UUID "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c"

const char* filename = "config_template.cfg";

void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step);
void moveMotorPercentage(Stepper stepper_obj, float percent, int *current_step);
void stopMotor(Stepper stepper_obj);
void ConnectToWiFi();
void serverStuff();

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


void loop() {
  /*
  int *joystick_values;
  if (current_step < 0) {
    current_step = 0;
  } else if (current_step > MAX_STEPS) {
    current_step = MAX_STEPS;
  }
  
  joystick_values = readJoystick();
  if (joystick_values[0] > 1500 || joystick_values[0] < -1500) {
    while (joystick_values[0] > 1500) {
      moveMotorSingleStep(myStepper, 1, &current_step);
      joystick_values = readJoystick();
    } 
    while (joystick_values[0] < -1500) {
      moveMotorSingleStep(myStepper, -1, &current_step);
      joystick_values = readJoystick();
    }
  }
  */
  ArduinoOTA.handle();
  serverStuff(myStepper);
}

int *readJoystick()
{

  static int  jvalues[10];
  int xval, yval, swval;
  swval = digitalRead(SW_pin);
  xval = analogRead(X_pin) - (JOYSTICK_MAX_ANALOG/2);
  yval = analogRead(Y_pin) - (JOYSTICK_MAX_ANALOG/2);
  jvalues[0] = xval;
  jvalues[1] = yval;
  jvalues[2] = swval;

  return jvalues;
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

void ConnectToWiFi()
{
 
  //ConnectToWiFi();
  WiFi.begin(SSID, WiFiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void serverStuff(Stepper stepper_obj)
{
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
            client.println(".slider { width: 300px; }</style>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
                     
            // Web Page
            client.println("</head><body><h1>ESP32 with Servo</h1>");
            client.println("<p>Position: <span id=\"servoPos\"></span></p>");          
            client.println("<input type=\"range\" min=\"0\" max=\"100\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");
            
            client.println("<script>var slider = document.getElementById(\"servoSlider\");");
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");
            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");
           
            client.println("</body></html>");     
            
            //GET /?value=180& HTTP/1.1
            if(header.indexOf("GET /?value=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              
              //Rotate the stepper
              moveMotorToPercentage(stepper_obj, valueString.toInt(), &current_step);
              Serial.println(valueString); 
              Serial.println((float)current_step / MAX_STEPS); 
            }         
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
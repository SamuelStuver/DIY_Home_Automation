#include <Arduino.h>
#include <Stepper.h>
#include <string.h>
#include <WiFi.h>

#define MAX_REVOLUTIONS 8
#define JOYSTICK_MAX_ANALOG 4095
#define SERVICE_UUID        "e6d0cf52-0695-43d1-bab8-5fcfe298a94a"
#define CHARACTERISTIC_UUID "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c"

void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step);
void moveMotorPercentage(Stepper stepper_obj, float percent, int *current_step);
void stopMotor(Stepper stepper_obj);

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


void setup() {

  Serial.begin(9600);

  // set EN pins for output
  //pinMode(enA, OUTPUT);
  //pinMode(enB, OUTPUT);
  //digitalWrite(enA, HIGH);
  //digitalWrite(enB, LOW);

  // set pins for analog stick
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  
  // set the motor speed:
  myStepper.setSpeed(speed);
  
}


void loop() {

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

  // digitalWrite(enA, HIGH);
  // digitalWrite(enB, HIGH);
  int n_steps = (MAX_STEPS * percent) / 100;
  Serial.print(*current_step);
  Serial.print(" out of  ");
  Serial.print(MAX_STEPS);
  Serial.print(" -->  ");
  Serial.print(n_steps);
  Serial.print("\n");
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
  Serial.print(*current_step);
  Serial.print(" out of  ");
  Serial.print(MAX_STEPS);
  Serial.print(" -->  ");
  Serial.print(n_steps);
  Serial.print("\n");

  // digitalWrite(enA, LOW);
  // digitalWrite(enB, LOW);
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
#include <Stepper.h>

#include <Arduino.h>
#include <Stepper.h>
#include <string.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define MAX_REVOLUTIONS 8
#define JOYSTICK_MAX_ANALOG 4095
#define SERVICE_UUID        "e6d0cf52-0695-43d1-bab8-5fcfe298a94a"
#define CHARACTERISTIC_UUID "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c"

void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step);
void moveMotorPercentage(Stepper stepper_obj, float percent, int *current_step);
void stopMotor(Stepper stepper_obj);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;



const int SW_pin = 13; // digital pin connected to switch output
const int X_pin = 32; // analog pin connected to X output
const int Y_pin = 33; // analog pin connected to Y output


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
const int MAX_STEPS = MAX_REVOLUTIONS * stepsPerRevolution;
// for your motor
int speed = 59;
float direction = 0.0;
int current_step = 0;

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 25, 26, 27, 14);


void setup() {

  Serial.begin(9600);
  // set pins for analog stick
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);

  Serial.println("Starting BLE work!");
  
  // set the motor speed:
  myStepper.setSpeed(speed);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  pCharacteristic->setValue((uint8_t*)"0", 2);
  
}

void loop() {
  char *characteristic_value;
  int *joystick_values;
  if (current_step < 0) {
    current_step = 0;
  } else if (current_step > MAX_STEPS) {
    current_step = MAX_STEPS;
  }
  
  joystick_values = readJoystick();
  while (joystick_values[0] > 1500) {
    moveMotorSingleStep(myStepper, 1, &current_step);
    joystick_values = readJoystick();
  } 
  while (joystick_values[0] < -1500) {
    moveMotorSingleStep(myStepper, -1, &current_step);
    joystick_values = readJoystick();
  }
  // notify changed value
  if (deviceConnected) {
      //pCharacteristic->setValue((uint8_t*)&value, 4);
      pCharacteristic->notify();
      characteristic_value = (char *)(pCharacteristic->getValue().c_str());
      //characteristic_data = pCharacteristic->getData();

      

      if (strcmp(characteristic_value, "0") != 0) {
        direction = atof(characteristic_value);
        moveMotorPercentage(myStepper, direction, &current_step);
      }
      else {
        stopMotor(myStepper);
      }
      
      pCharacteristic->setValue((uint8_t*)"0", 2);
      //Serial.println(direction);
      
      delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
  delay(10);
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
  
}

void moveMotorSingleStep(Stepper stepper_obj, int sign, int *current_step)
{
  Serial.print(*current_step);
  Serial.print(" out of  ");
  Serial.print(MAX_STEPS);
  Serial.print(" -->  ");
  Serial.print(sign);
  Serial.print("\n");
  int n_steps = 10;
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
  
}

void stopMotor(Stepper stepper_obj)
{
  stepper_obj.step(0);
}
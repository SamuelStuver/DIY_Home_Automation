# 1 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino"
# 1 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino"
# 2 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2

# 4 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 5 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 6 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 7 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 8 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 9 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 10 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2
# 11 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 2

#define MAX_REVOLUTIONS 8
#define JOYSTICK_MAX_ANALOG 4095
#define SERVICE_UUID "e6d0cf52-0695-43d1-bab8-5fcfe298a94a"
#define CHARACTERISTIC_UUID "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c"

BLEServer* pServer = 
# 17 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 3 4
                    __null
# 17 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino"
                        ;
BLECharacteristic* pCharacteristic = 
# 18 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino" 3 4
                                    __null
# 18 "c:\\Users\\samue\\Documents\\Window_Blind_Turner_Newest\\Window_Blind_Turner\\Window_Blind_Turner.ino"
                                        ;
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


const int stepsPerRevolution = 200; // change this to fit the number of steps per revolution
const int max_steps = 8 * stepsPerRevolution;
// for your motor
int speed = 59;
double direction = 0.0;
int c;




// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 25, 26, 27, 14);


void setup() {

  Serial.begin(9600);
  // set pins for analog stick
  pinMode(SW_pin, 0x01);
  digitalWrite(SW_pin, 0x1);

  Serial.println("Starting BLE work!");

  // set the motor speed:
  myStepper.setSpeed(speed);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService("e6d0cf52-0695-43d1-bab8-5fcfe298a94a");

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      "f05e2e6a-b8b7-4e12-b7c8-d2dda61d2e5c",
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("e6d0cf52-0695-43d1-bab8-5fcfe298a94a");
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

}

void loop() {
  char *characteristic_value;
  int *joystick_values;

  // notify changed value
  if (deviceConnected) {
      //pCharacteristic->setValue((uint8_t*)&value, 4);
      pCharacteristic->notify();
      characteristic_value = (char *)(pCharacteristic->getValue().c_str());
      //characteristic_data = pCharacteristic->getData();

      joystick_values = readJoystick();
      Serial.print("x: ");
      Serial.print(joystick_values[0]);
      Serial.print("  y: ");
      Serial.print(joystick_values[1]);
      Serial.print("  sw: ");
      Serial.print(joystick_values[2]);
      Serial.print("\n");
      while (joystick_values[0] > 1500) {
        myStepper.step(10);
        joystick_values = readJoystick();
      }
      while (joystick_values[0] < -1500) {
        myStepper.step(-10);
        joystick_values = readJoystick();
      }

      if (atof(characteristic_value) > 0) {
      Serial.println("Rotating Clockwise");
      direction = atof(characteristic_value);
      Serial.println(atof(characteristic_value));
      Serial.println(" <- direction");
      myStepper.step(max_steps * direction / 100.);
      }
      else if (atof(characteristic_value) < 0) {
        Serial.println("Rotating Counter-Clockwise");
        direction = atof(characteristic_value);
        Serial.println(" <- direction");
        Serial.println(atof(characteristic_value));
        myStepper.step(max_steps * direction / 100.);
      }
      else {
        direction = 0.0;
        myStepper.step(1 * direction / 100.);
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

  static int jvalues[10];
  int xval, yval, swval;
  swval = digitalRead(SW_pin);
  xval = analogRead(X_pin) - (4095/2);
  yval = analogRead(Y_pin) - (4095/2);
  jvalues[0] = xval;
  jvalues[1] = yval;
  jvalues[2] = swval;

  return jvalues;
}

/**
 * Bike Thing by Ethan Fox and Jennifer Lee for ECE 4180
 * Modified Template of BLE_Client for the Arduino Nano ESP32
 * Much of the bluetooth communication was adapted for our usecases. 
 * We have included comments where we have made edits in the bluetooth code.
 */

int upperPackData = 0;
#include "BLEDevice.h"
#include <FastLED.h>

//#include "BLEScan.h"
#define DISTTHRESH 1000
#define LED_PIN_L 14
#define LED_PIN_R 13
#define NUM_LEDS  23
#define DURATION 500

#define TURNSTART 18
volatile bool isFlipped = false;
volatile bool isBraking = false;
volatile bool turnLeft = true;
volatile bool turnRight = true;

CRGB left_led[NUM_LEDS];
CRGB right_led[NUM_LEDS];




// The following are all constants required for bluetooth communication

// The remote service we wish to connect to.
static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    charUUID2("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic2;
static BLEAdvertisedDevice* myDevice;

// This is the callback handler for whenever we receive data
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // This callback function was modified to receive data in the format which it was sent from the MBED
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.print((int) *pData);
    Serial.write(pData, length);
    upperPackData = *pData;
    turnRight = 1 & upperPackData;
    turnLeft = 4 & upperPackData;
    isBraking = !(2 & upperPackData);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// This gets called once we find a bluetooth server with our desired address (this is the address of the mbed pack's BLE peripheral)
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify()){
      Serial.println("Callback registered.");

      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
    // We added a second bluetooth le characteristic. One for sending and one for receiving.
    // We ended up not using the first one due to the Blind Spot system not working.
    if (pRemoteCharacteristic2 == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID2.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic2->canRead()) {
      std::string value = pRemoteCharacteristic2->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic2->canNotify())
      pRemoteCharacteristic2->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(9600);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
  
  Serial.println("Starting Arduino BLE Client application...");

  // We add our LEDs here
  FastLED.addLeds<WS2812, LED_PIN_L, GRB>(left_led, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_PIN_R, GRB>(right_led, NUM_LEDS);

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  
  
} // End of setup.

// Below is the code we have added for the logic behind the blinking lights

bool flip = false;
int prev_millis = 0;
volatile int dist = 0;


// every loop, this sets up what should be displayed on the light
void update_lights() {
  for(int i = 0; i < NUM_LEDS; i++) {
    left_led[i] = right_led[i] = CRGB::Black;
  }

  for(int i = 0; i < TURNSTART; i++) {
    left_led[i] = right_led[i] = CRGB::Red;
  }
  
  if (isBraking) {
    FastLED.setBrightness(255 / 8);
  } else {
    FastLED.setBrightness(255);
  }

  for (int i = TURNSTART; i < NUM_LEDS; i++) {
    if (flip) {
      if (turnLeft) left_led[i] = CRGB::Orange;
      if (turnRight) right_led[i] = CRGB::Orange;
    }
  }
  
  FastLED.show();
}


void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    Serial.println("Sending \"N'");    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic2->writeValue("N", 1);
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  update_lights();
  delay(250); // Delay a second between loops.
  flip = !flip;


} // End of loop

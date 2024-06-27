/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect handler associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

BLEAdvertising *pAdvertising = NULL;
char mfgData[8] = {0xFF, 0xAB, 0x01, 0 x02, 0x00, 0x00, 0x00, 0x01};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("MR2");

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  BLEDevice::startAdvertising();
}

void loop() {
  mfgData[3] = (mfgData[3] + 1);
  mfgData[5] = (mfgData[3] + 1);

  BLEAdvertisementData advertisementData = BLEAdvertisementData();
  advertisementData.setManufacturerData(String(mfgData, sizeof(mfgData)));
  pAdvertising->setAdvertisementData(advertisementData);

  delay(100);
}

#include <Arduino.h>
#include <BLEDevice.h>
#include <vector>

#include "auxiliary.h"
#include "bluetooth.hpp"

#define SCAN_DURATION 1
#define SCAN_WINDOW 100
#define SCAN_INTERVAL 500

// RSSI thresholds
#define RSSI_CONNECT -80
#define RSSI_DISCONNECT -90

uuids UUID_generator;

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice *peripheral_device;
BLEScan *pBLEScan;
std::vector<BLEClient *> clients; // one BLEClient object per connected server

hw_timer_t *timer_5sec = NULL;

void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
                           bool isNotify)
{
    Serial.print("Notify data from peripheral: ");
    // print entire recieved packet by byte
    for (int i = 0; i < length; i++)
    {
        Serial.print(*(pData + i), HEX);
    }
    Serial.print(". Length: ");
    Serial.println(length);
}

bool connectToServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(peripheral_device->getAddress().toString().c_str());

    BLEClient *pClient = BLEDevice::createClient();
    clients.push_back(pClient);

    pClient->setClientCallbacks(new ClientCallback());

    // Connect to the remote BLE Server.
    pClient->connect(peripheral_device);
    Serial.println(" - Connected to server");
    pClient->setMTU(40); // arbitrary MTU. The max is supposedly 517

    // prep primary service object
    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }

    // prep characteristic object (Just one for now. Later: add multiple for sharing other info (battery status etc.))
    BLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }

    // prep to receive notifications from peripheral
    pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    digitalWrite(LED_BUILTIN, HIGH);
    return true;
}

void scan()
{
    pBLEScan->start(SCAN_DURATION, false);
}

void disconnect_bad_servers()
{
    BLEClient *client;
    for (int i = 0; i < clients.size(); i++)
    {
        client = clients[i];
        if (client->getRssi() < RSSI_DISCONNECT)
        {
            Serial.println("RSSI fell below threshold. Disconnecting from server.");
            client->disconnect();
            clients.erase(clients.begin() + i);
            i--;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    UUID_generator.initialize_random_values();
    UUID_generator.generate_hashes();
    Serial.printf("SERVICE UUID - %s\n", UUID_generator.get_service_uuid());
    Serial.printf("SERVICE UUID - %s\n", UUID_generator.get_characteristic_uuid());

    BLEDevice::init("AGGREGATOR");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setInterval(SCAN_INTERVAL);
    pBLEScan->setWindow(SCAN_WINDOW);
    pBLEScan->setActiveScan(true);

    scan();
}

void loop()
{
    if (doConnect)
    {
        Serial.println("Trying to connect...");
        if (connectToServer())
        {
            Serial.println("Connected!");
        }
        else
        {
            Serial.println("Connection failed.");
        }
        doConnect = false;
    }
    // ALWAYS scan, because devices could become available at any time

    scan();
    disconnect_bad_servers(); // ultimately make this asynchronous with FreeRTOS task
}

#include <Arduino.h>
#include <BLEDevice.h>
#include <vector>

#define SCAN_DURATION 1
#define SCAN_WINDOW 100
#define SCAN_INTERVAL 500

// RSSI thresholds
#define RSSI_CONNECT -80
#define RSSI_DISCONNECT -90
#define DEVICE_NAME "REPEATER"

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHARACTERISTIC_UUID);

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice *peripheral_device;
BLEScan *pBLEScan;
std::vector<BLEClient *> clients; // one BLEClient object per connected server

hw_timer_t *timer_5sec = NULL;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
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

class ClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        connected = false;
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("onDisconnect...");
    }
};

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

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /**
     * Called for each advertising BLE server.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // Only connect to the peripheral if:
        // - correct service is advertised
        // - RSSI is high enough
        // Serial.println(advertisedDevice.toString().c_str());
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) &&
            (advertisedDevice.getRSSI() >= RSSI_CONNECT))
        {
            Serial.println(advertisedDevice.haveServiceUUID());
            peripheral_device = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            Serial.println("Peripheral found.");
        }
        else
        {
        }
    }
};

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

    Serial.begin(115200); // Debug output
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("THIS IS THE CLIENT/RECEIVER");
    BLEDevice::init(DEVICE_NAME);
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setInterval(SCAN_INTERVAL);
    pBLEScan->setWindow(SCAN_WINDOW);
    pBLEScan->setActiveScan(true);

    scan(); // initial scan
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
    delay(5000);
}

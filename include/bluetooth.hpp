#ifndef A3A894A8_7557_423C_AF30_DF2AD1257EE6
#define A3A894A8_7557_423C_AF30_DF2AD1257EE6

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <sstream>

#define RSSI_CONNECT -80
#define RSSI_DISCONNECT -90
#define SCAN_DURATION 1
#define SCAN_WINDOW 100
#define SCAN_INTERVAL 500

// globals BAD
bool clientConnected = false;
bool serverConnected = false;
boolean doServerConnect = false;
BLEAdvertisedDevice *peripheral_device;

typedef struct __attribute__((__packed__)) BluetoothTransmissionData
{
    float temp_data;
    float latitude;
    float longitude;
    float altitude;
} BluetoothTransmissionData_t;

typedef union BluetoothTransmissionDataConverter_u {

    BluetoothTransmissionData_t message;
    uint8_t bytes[sizeof(BluetoothTransmissionData)];

} BluetoothTransmissionDataConverter_t;

class CharacteristicCallbacks : public BLECharacteristicCallbacks
{
  private:
    BluetoothTransmissionData _data;

  public:
    CharacteristicCallbacks()
    {
    }

    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0)
        {
            Serial.println("*********");
            Serial.print("New value: ");
            for (int i = 0; i < value.length(); i++)
                Serial.print(value[i]);

            Serial.println();
            Serial.println("*********");
        }
    }

    void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        std::stringstream str_data("");
        str_data << "TEMP: " << _data.temp_data << " LAT: " << _data.latitude << " LONG: " << _data.longitude
                 << " ALT: " << _data.altitude;

        Serial.println(str_data.str().c_str());

        pCharacteristic->setValue(str_data.str());
    }

    void onNotify(BLECharacteristic *pCharacteristic, uint8_t *pData, size_t length,
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

    void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code)
    {
        printf("STATUS\n");
    }

    BluetoothTransmissionData getData()
    {
        return _data;
    }

    void setData(BluetoothTransmissionData data)
    {
        _data = data;
    }
};

class ServerCallbacks : public BLEServerCallbacks
{
  public:
    void onConnect(BLEServer *pServer)
    {
        clientConnected = true;
        Serial.printf("Connected....\n");
        BLEDevice::stopAdvertising();
    }

    void onDisconnect(BLEServer *pServer)
    {
        clientConnected = false;
        Serial.printf("Disconnected....\n");
        BLEDevice::startAdvertising();
    }
};

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
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) &&
            (advertisedDevice.getRSSI() >= RSSI_CONNECT))
        {
            Serial.println(advertisedDevice.haveServiceUUID());
            peripheral_device = new BLEAdvertisedDevice(advertisedDevice);
            doServerConnect = true;
            Serial.println("Peripheral found.");
        }
        else
        {
        }
    }
};

class ClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        serverConnected = false;
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("onDisconnect...");
    }
};

template <typename _UUID_Generator_Type> class Bluetooth
{
  private:
    BLEServer *pServer;
    BLEService *pService;
    BLECharacteristic *pCharacteristic;
    BLEAdvertising *pAdvertising;
    _UUID_Generator_Type _uuid_gen_struct;
    BLEScan *pBLEScan;
    std::vector<BLEClient *> clients; // one BLEClient object per connected server

  public:
    CharacteristicCallbacks *callback_class;

    Bluetooth()
    {
    }

    Bluetooth(_UUID_Generator_Type uuid_gen_struct) : _uuid_gen_struct(uuid_gen_struct)
    {
        callback_class = new CharacteristicCallbacks();

        BLEDevice::init("A0");
        pServer = BLEDevice::createServer();
        pService = pServer->createService(_uuid_gen_struct.get_service_uuid());
        pCharacteristic = pService->createCharacteristic(
            _uuid_gen_struct.get_characteristic_uuid(),
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

        pCharacteristic->setCallbacks(callback_class);
        pCharacteristic->setValue("Sensor Node");
        pService->start();

        pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(_uuid_gen_struct.get_service_uuid());
        pAdvertising->setScanResponse(false);
        pAdvertising->setMinPreferred(0x0);
        pAdvertising->start();
        // will add semaphore so advertising and scanning can't be simultaneous
        BLEDevice::init("A0");
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
        pBLEScan->setInterval(SCAN_INTERVAL);
        pBLEScan->setWindow(SCAN_WINDOW);
        pBLEScan->setActiveScan(true);

    }

    void sendData()
    {
        if (clientConnected)
        {
            BluetoothTransmissionDataConverter_t converter;
            converter.message = callback_class->getData();
            pCharacteristic->setValue(converter.bytes, sizeof(BluetoothTransmissionData));
            pCharacteristic->notify();
        }
    }

    bool tryConnectToServer()
    {
        if (doServerConnect)
        {
            doServerConnect = false;
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

            serverConnected = true;
            digitalWrite(LED_BUILTIN, HIGH);
            return true;
        }

        return false;
    }

    void scan()
    {
        pBLEScan->start(SCAN_DURATION, false);
    }
    
    void removeOldServers()
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
};

#endif /* A3A894A8_7557_423C_AF30_DF2AD1257EE6 */

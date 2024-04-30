#ifndef A3A894A8_7557_423C_AF30_DF2AD1257EE6
#define A3A894A8_7557_423C_AF30_DF2AD1257EE6

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <sstream>
#include <queue>
#include "data_packet.h"

#define RSSI_CONNECT -80
#define RSSI_DISCONNECT -90
#define SCAN_DURATION 1
#define SCAN_WINDOW 100
#define SCAN_INTERVAL 500

// globals BAD
bool serverConnected = false;
boolean doServerConnect = false;
BLEAdvertisedDevice *peripheral_device;

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
        // Match sensors based on name
        if (advertisedDevice.haveName() && advertisedDevice.getName() == "A0" && (advertisedDevice.getRSSI() >= RSSI_CONNECT))
        {
            peripheral_device = new BLEAdvertisedDevice(advertisedDevice);
            doServerConnect = true;
            Serial.println("Peripheral found.");
        }
        else
        {
        }
    }
};

// Functions for connection/disconnection with aggregators
class ClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        Serial.println("An aggregator connected.");
    }

    void onDisconnect(BLEClient *pclient)
    {
        serverConnected = false;
        Serial.println("An aggregator disconnected.");
    }
};

// main class for handling BLE 
template <typename _UUID_Generator_Type> class Bluetooth
{
  private:
    _UUID_Generator_Type _uuid_gen_struct;
    BLEScan *pBLEScan;
    std::vector<BLEClient *> clients; // one BLEClient object per connected server

  public:
    Bluetooth()
    {
    }

    Bluetooth(_UUID_Generator_Type uuid_gen_struct) : _uuid_gen_struct(uuid_gen_struct)
    {
        // initialize as client scanning for servers
        BLEDevice::init("R0");
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
        pBLEScan->setInterval(SCAN_INTERVAL);
        pBLEScan->setWindow(SCAN_WINDOW);
        pBLEScan->setActiveScan(true);

    }

    // Run whenever a message is received from an aggregator
    static void clientOnNotify(BLERemoteCharacteristic *pCharacteristic, uint8_t *pData, size_t length,
                           bool isNotify)
    {
        TransmissionData_t *data = new TransmissionData_t;
        float temp = *((float *)pData);
        data->temp_data = temp; 
        received_packets.push(data);
    }

    bool tryConnectToServer()
    {
        if (doServerConnect)
        {
            doServerConnect = false;
            Serial.print("Forming a connection to ");
            Serial.println(peripheral_device->getAddress().toString().c_str());

            BLEClient *pClient = BLEDevice::createClient();

            pClient->setClientCallbacks(new ClientCallback());

            // Connect to the remote BLE Server.
            pClient->connect(peripheral_device);
            Serial.println(" - Connected to server");
            pClient->setMTU(40); // arbitrary MTU. The max is supposedly 517

            // prep primary service object
            BLERemoteService *pRemoteService = pClient->getService(_uuid_gen_struct.get_service_uuid());
            if (pRemoteService == nullptr)
            {
                Serial.println("Incorrect service UUID");
                pClient->disconnect();
                return false;
            }

            // prep characteristic object (Just one for now. Later: add multiple for sharing other info (battery status etc.))
            BLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(_uuid_gen_struct.get_characteristic_uuid());
            if (pRemoteCharacteristic == nullptr)
            {
                Serial.println("Incorrect characteristic UUID");
                pClient->disconnect();
                return false;
            }

            clients.push_back(pClient);
            // prep to receive notifications from peripheral
            pRemoteCharacteristic->registerForNotify(clientOnNotify);

            serverConnected = true;
            return true;
        }

        return false;
    }

    // Scan for available aggregators
    void scan()
    {
        pBLEScan->start(SCAN_DURATION, false);
    }
    
    // Disconnect servers from the list that are no longer connected or are already disconnected
    void removeOldServers()
    {
        // one client object per connected server. So each represents a server. (Confusing.)
        BLEClient *client;
        for (int i = 0; i < clients.size(); i++)
        {
            client = clients[i];
            if (client->getRssi() < RSSI_DISCONNECT) // remove clients with poor connection
            {
                Serial.println("RSSI fell below threshold. Disconnecting from server.");
                client->disconnect();
                clients.erase(clients.begin() + i);
                i--;
            } else if (!client->isConnected()) { // remove already disconnected clients
                clients.erase(clients.begin() + i);
                i--;
            }
        }
    }
};

#endif /* A3A894A8_7557_423C_AF30_DF2AD1257EE6 */

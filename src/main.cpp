#include <Arduino.h>
#include <BLEDevice.h>
#include <vector>

#include "auxiliary.h"
#include "bluetooth.hpp"


uuids UUID_generator;
Bluetooth<uuids> bluetooth;

void setup()
{
    Serial.begin(115200);
    UUID_generator.initialize_random_values();
    UUID_generator.generate_hashes();
    Serial.printf("SERVICE UUID - %s\n", UUID_generator.get_service_uuid());
    Serial.printf("CHARACTERISTIC UUID - %s\n", UUID_generator.get_characteristic_uuid());
    bluetooth = Bluetooth<uuids>(UUID_generator);
}

void loop()
{
    if (bluetooth.clientIsConnected()) // only worry about finding sensors after repeater is connected
    {
        bluetooth.tryConnectToServer();
        // ALWAYS scan, because devices could become available at any time
        bluetooth.scan();
        bluetooth.removeOldServers();
    }
}

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
    pinMode(LED_BUILTIN, OUTPUT);
    //UUID_generator.initialize_random_values();
    //UUID_generator.generate_hashes();
    Serial.printf("SERVICE UUID - %s\n", UUID_generator.get_service_uuid());
    Serial.printf("CHARACTERISTIC UUID - %s\n", UUID_generator.get_characteristic_uuid());
    bluetooth = Bluetooth<uuids>(UUID_generator);
}

void loop()
{
    if (bluetooth.clientIsConnected()) // only worry about finding sensors after repeater is connected
    {
        digitalWrite(LED_BUILTIN, HIGH);
        if (received_packets.size() > 0)
        {
            // admittedly cursed
            bluetooth.callback_class->setData(* received_packets.front());
            received_packets.pop();
            bluetooth.sendData();
        }
        bluetooth.tryConnectToServer();
        // ALWAYS scan, because devices could become available at any time
        bluetooth.scan();
        bluetooth.removeOldServers();
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

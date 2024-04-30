#include <Arduino.h>
#include <BLEDevice.h>
#include <vector>
#include "esp_task_wdt.h"

#include "auxiliary.h"
#include "bluetooth.hpp"
#include "hardware_timer.hpp"
#include "acoustic.hpp"


uuids UUID_generator;
Bluetooth<uuids> bluetooth;

volatile SemaphoreHandle_t scan_semaphore;
volatile SemaphoreHandle_t disconnect_semaphore;
volatile int8_t SCAN_ISR = 0;
volatile int8_t DISCON_ISR = 0;
portMUX_TYPE isr_mux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t acoustic_task_handle = NULL;
TaskHandle_t main_task_handle = NULL;

std::queue<TransmissionData_t *> received_packets;

void main_loop(void *args);

void ARDUINO_ISR_ATTR set_semaphore()
{
    taskENTER_CRITICAL_ISR(&isr_mux);
    if (DISCON_ISR == 20)
    {
        xSemaphoreGiveFromISR(disconnect_semaphore, NULL);
    }
    else
    {
        DISCON_ISR++;
    }
    if (SCAN_ISR == 6)
    {
        xSemaphoreGiveFromISR(scan_semaphore, NULL);
    }
    else
    {
        SCAN_ISR++;
    }
    taskEXIT_CRITICAL_ISR(&isr_mux);
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    //UUID_generator.initialize_random_values();
    //UUID_generator.generate_hashes();
    Serial.printf("SERVICE UUID - %s\n", UUID_generator.get_service_uuid());
    Serial.printf("CHARACTERISTIC UUID - %s\n", UUID_generator.get_characteristic_uuid());
    bluetooth = Bluetooth<uuids>(UUID_generator);
    
    scan_semaphore = xSemaphoreCreateBinary();
    disconnect_semaphore = xSemaphoreCreateBinary();
    setup_timer(set_semaphore, 250, 80);

    esp_task_wdt_init(UINT32_MAX, false); // make task_wdt wait as long as possible, until we come up with better solution
    xTaskCreatePinnedToCore(acoustic_receive_loop, "acoustic_receive_loop", 4096, NULL, 19, &acoustic_task_handle, 0); // priority at least 19
    xTaskCreatePinnedToCore(main_loop, "main_loop", 4096*4, NULL, 19, &main_task_handle, 1); // priority at least 19
}

void main_loop(void *args)
{
    while (1)
    {
        bluetooth.removeOldServers();

        if (xSemaphoreTake(scan_semaphore, 0) == pdTRUE)
        {
            bluetooth.scan();
            bluetooth.tryConnectToServer();
        }

        if (xSemaphoreTake(disconnect_semaphore, 0) == pdTRUE )
        {
            bluetooth.removeOldServers();
        }
    }
}

void loop()
{

}

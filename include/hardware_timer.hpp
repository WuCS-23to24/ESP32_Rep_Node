#ifndef B944A19B_5CF1_40F8_A95D_CF2AA3B980F8
#define B944A19B_5CF1_40F8_A95D_CF2AA3B980F8

#include <Arduino.h>
#include <esp_system.h>

const int wdtTimeout = 250; // time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void ARDUINO_ISR_ATTR check_gps_data()
{
    ets_printf("ISR TRIGGERED\n");
}

void setup_timer(void (*func)(void) = check_gps_data)
{
    timer = timerBegin(0, 80, true);                    // timer 0, div 80
    timerAttachInterrupt(timer, func, true);           // attach callback
    timerAlarmWrite(timer, wdtTimeout * 1000, false);   // set time in us
    timerSetAutoReload(timer, true);
    timerAlarmEnable(timer);
}

#endif /* B944A19B_5CF1_40F8_A95D_CF2AA3B980F8 */

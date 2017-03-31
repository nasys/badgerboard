#include <badger.h>
/* 5 * 60 * 1000UL defines interval when temperature is sent 
 * 30 * 1000UL defines offset from beginning
 *  PRINT_ENABLE - prints to serial port " NEXT TX IN ..."
 *  PRINT_DISABLE - disables " NEXT TX IN ..." message
 */
 
badger_scheduler cayanne_sched(5 * 60 * 1000UL, 30 * 1000UL, PRINT_ENABLE); 
badger_scheduler status_sched(10 * 60 * 1000UL, 150 * 1000UL, PRINT_ENABLE);

uint32_t s_badger_sleep_period_ms = 32 * 1000UL;


// enter your own key

const uint8_t devEUI[8] = {

};

const uint8_t appKey[16] = {

};

const uint8_t appEUI[8] = {

};

void setup() 
{
    badger_init();
    Serial.println("badger temp/hum sensor");
    badger_print_EUI(devEUI);
    LoRa_init(devEUI, appEUI, appKey);
    
}

void loop()
{
    if(cayanne_sched.run_now())
    {
       sendCayanneTempHum();
    }
    
    status_sched.print_soonest();
    if(Lora_requires_reset())
    {
        badger_restart();        
    }
    badger_sleep_now(s_badger_sleep_period_ms);
}


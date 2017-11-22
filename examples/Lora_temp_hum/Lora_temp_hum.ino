#include <badger.h>
/* 5 * 60 * 1000UL defines interval when temperature is sent 
 * 30 * 1000UL defines offset from beginning
 *  PRINT_ENABLE - prints to serial port " NEXT TX IN ..."
 *  PRINT_DISABLE - disables " NEXT TX IN ..." message
 */
 
badger_scheduler temp_sched(5 * 60 * 1000UL, 30 * 1000UL, PRINT_ENABLE); 
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
    LoRa_init(devEUI, appEUI, appKey,true);
    
}

void loop()
{
    if(temp_sched.run_now())
    {
        // this function is called when the time defined at the "temp_sched" variable is reached
        badger_pulse_led(1);
        badger_blink_error(badger_temp_sensor_send());
    }
    if(status_sched.run_now())
    {
        badger_pulse_led(1);
        badger_blink_error(badger_temp_sensor_send_status(0));
    }
    status_sched.print_soonest();
    if(Lora_requires_reset())
    {
        badger_restart();        
    }
    badger_sleep_now(s_badger_sleep_period_ms);
}


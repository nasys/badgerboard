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
const uint8_t devAddr[8] = { 0x00, 0x1b, 0x70, 0xe6};    //001B70E6   //0x0558ef68        0x12345678

const uint8_t ntwkSKey[16] = { 0x48, 0x45, 0x5B, 0xF8, 0x6E, 0x44, 0x6B, 0x58, 0xC8, 0xC4, 0xBA, 0x03, 0x52, 0x4C, 0x2C, 0xE6 };//48455BF86E446B58C8C4BA03524C2CE6

const uint8_t appSKey[16] = { 0x70, 0x3F, 0xAC, 0xAC, 0x50, 0xE0, 0xEA, 0x05, 0xAE, 0x15, 0x0C, 0x36, 0xBA, 0x9F, 0xFE, 0x5B };

void setup() 
{
    badger_init();
    Serial.println("badger temp/hum sensor");
    badger_print_EUI(devEUI);
    //LoRa_init(devEUI, appEUI, appKey);
    LoRa_initABP(devAddr, appSKey, ntwkSKey, false);
    
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


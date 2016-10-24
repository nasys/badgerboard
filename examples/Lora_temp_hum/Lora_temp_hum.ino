#include <badger.h>

badger_scheduler temp_sched(5 * 60 * 1000UL, 30 * 1000UL, PRINT_ENABLE);
badger_scheduler status_sched(10 * 60 * 1000UL, 60 * 1000UL, PRINT_ENABLE);

uint32_t s_badger_sleep_period_ms = 32 * 1000UL;
uint32_t s_LoRa_tx_period_min = 30 * 1000UL;

// enter your own key
const uint8_t devEUI[8] = {

};

const uint8_t appEUI[8] = {
 
};

const uint8_t appKey[16] = {

};

void setup() 
{
    badger_init();
    LoRa_tx_interval_min_set(s_LoRa_tx_period_min);

    Serial.println("32U4 temp/hum sensor");
    badger_print_EUI(devEUI);
    LoRa_enable_long_range();
    LoRa_init(devEUI, appEUI, appKey);
    LoRa_enable_long_range();
    LoRa_sleep();
}

void loop()
{
    if(temp_sched.run_now())
    {
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


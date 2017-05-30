#include <badger.h>

bool LoRa_add_sensitivity(uint8_t sensitivity);

badger_scheduler temp_sched(1 * 60 * 1000UL, 30 * 1000UL, PRINT_ENABLE);

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
    badger_print_EUI(devEUI);
    LoRa_init(devEUI, appEUI, appKey);
}

void loop()
{
int serial_input  = Serial.read()- 0x30;
if(serial_input > 0)
  {
    digitalWrite(A5,HIGH);
    Serial.print(serial_input);
    LoRa_add_sensitivity(serial_input);
    digitalWrite(A5,LOW);
  }
if(temp_sched.run_now())
  {
    // this function is called when the time defined at the "temp_sched" variable is reached
    badger_pulse_led(1);
    badger_blink_error(badger_temp_send());
  }
temp_sched.print_soonest();
if(Lora_requires_reset())
  {
    badger_restart();        
  }
  badger_sleep_now(1000);
}


//drowning sensor
#include <badger.h>

// add your own key
const uint8_t devEUI[8] = {

};

const uint8_t appKey[16] = {

};

const uint8_t appEUI[8] = {

};

badger_scheduler status_sched(10 * 60 * 1000UL, 60 * 1000UL, PRINT_ENABLE);

void setup() 
{
// Arduino Pin Setup

    pinMode(A0,OUTPUT);
    pinMode(A1,INPUT);
    digitalWrite(A0,LOW);
    Serial.begin(9600);

// badger and LoRa Setup

    badger_init();
    Serial.println("drowning sensor");
    badger_print_EUI(devEUI);
    LoRa_init(devEUI, appEUI, appKey);
}

bool water_level() //read analog value and decide if sensor is in water or not
{
    digitalWrite(A0,HIGH);
    uint16_t drowning_value = analogRead(A1);
    Serial.print("drowning value :  ");
    Serial.println(drowning_value);
    digitalWrite(A0,LOW);
    if(drowning_value > 300)
    {
        return true;
    }

    if(drowning_value <= 300)
    {
        return false;
    }

}


void loop()
{
    bool badger_drowning = water_level();

    if(badger_drowning == false)
    {
        LoRa_send(30,(uint8_t*)"OK",2,3); 
        Serial.println("everything is OK ");
    }
    else
    {
        LoRa_send(30,(uint8_t*)"Drowning",8,3);
        Serial.println("your sensor is in water");        
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

    badger_sleep_now(30 * 1000UL);
}

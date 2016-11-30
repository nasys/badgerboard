//drowning sensor
#include <badger.h>


const uint8_t devEUI[8] = {
0x11, 0x11, 0x22, 0x22,
0x33, 0x33, 0x44, 0x44
};

const uint8_t appKey[16] = {
0x11, 0x11, 0x11, 0x11,
0x11, 0x11, 0x22, 0x22,
0x22, 0x22, 0x22, 0x22,
0x33, 0x33, 0x33, 0x33
};

const uint8_t appEUI[8] = {
0x11, 0x22, 0x33, 0x44,
0x55, 0x66, 0x77, 0x88
};


badger_scheduler status_sched(1 * 60 * 1000UL, 60 * 1000UL, PRINT_ENABLE);


uint32_t s_LoRa_tx_period_min = 30 * 1000UL;

void setup() 
{
// Arduino Pin Setup

pinMode(A0,OUTPUT);
pinMode(A1,INPUT);
digitalWrite(A0,LOW);
Serial.begin(9600);

// badger and LoRa Setup

badger_init();
LoRa_tx_interval_min_set(s_LoRa_tx_period_min);
Serial.println("drowning sensor");
badger_print_EUI(devEUI);
LoRa_init(devEUI, appEUI, appKey);
LoRa_sleep();
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
static uint8_t x = 0;
bool badger_status = water_level();
    
if(status_sched.run_now())
  {
    if(badger_status == false)
     x = 0;
    if(badger_status == true)
      x = 1;
      Serial.print("x value");
      Serial.println(x);
      LoRa_send(30,(uint8_t*)"Tere raisk",10,1);
   }
status_sched.print_soonest();
if(Lora_requires_reset())
  {
    badger_restart();        
  }
delay(1000); 
if(badger_status == true)
  Serial.println("your sensor is in water");
else
  Serial.println("everything is OK ");

}

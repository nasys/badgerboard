#include <badger.h>

/* 5 * 60 * 1000UL defines interval when temperature is sent 
 * 30 * 1000UL defines offset from beginning
 *  PRINT_ENABLE - prints to serial port " NEXT TX IN ..."
 *  PRINT_DISABLE - disables " NEXT TX IN ..." message
 */

badger_scheduler temp_sched(1 * 50 * 1000UL, 30 * 1000UL, PRINT_ENABLE); 
badger_scheduler status_sched(5 * 60 * 1000UL, 150 * 1000UL, PRINT_ENABLE);
badger_scheduler temp_only_shced(35 * 60 * 1000UL, 120 * 1000UL, PRINT_ENABLE);
uint32_t s_badger_sleep_period_ms = 32 * 1000UL;


// enter your own key

const uint8_t devEUI[8] = {
0x70, 0xb3, 0xd5, 0xb0,
0x20, 0x00, 0x07, 0x79
};

const uint8_t appKey[16] = {
0x67, 0xf0, 0x5b, 0xef,
0xd8, 0xa9, 0xdb, 0x81,
0x2d, 0x6d, 0x20, 0xf0,
0xc0, 0x5c, 0x0c, 0x1a
};


const uint8_t appEUI[8] = {
0x70, 0xb3, 0xd5, 0xb0,
0x20, 0x00, 0x00, 0x02
};

void setup() 
{
    badger_init();
    Serial.println("badger temp/hum sensor");
    badger_print_EUI(devEUI);
    LoRa_init(devEUI, appEUI, appKey);
    pinMode(A1,OUTPUT);
    digitalWrite(A1,HIGH);
    delay(1000);
    digitalWrite(A1,LOW);
}

void loop()
{
  static uint16_t last_receive_time = 0;
  uint8_t Lora_receive_buffer[80];
  uint8_t last_received_numeric= 0;
  static uint32_t time_now = 0;
  static uint32_t time_last_packet = 0;
  static uint16_t receiveSize = 0;
  uint16_t received_number = 0;
    
int serial_input  = Serial.read()- 0x30;
if(serial_input > 0)
{
  Serial.print(serial_input);
  LoRa_add_sensitivity(serial_input);
  
}

    if(temp_sched.run_now())
    {
        // this function is called when the time defined at the "temp_sched" variable is reached
        
        badger_pulse_led(1);
        badger_blink_error(badger_temp_send());
    memset(Lora_receive_buffer, 0 , sizeof(Lora_receive_buffer));
    receiveSize = LoRa_receive(Lora_receive_buffer,sizeof(Lora_receive_buffer),0);

        if (receiveSize != 0)
        {
            time_last_packet = millis();
            Serial.println("Packet received!");
        }
    }
        
        
        if (receiveSize > 0 && millis() - time_last_packet > 10000)
        {
            Serial.print("Relaying received data.");
       
            if(receiveSize == sizeof(received_number))
            {
              uint8_t* data_ptr = (uint8_t*) &received_number;
              memcpy(data_ptr, Lora_receive_buffer, receiveSize);           
              char json_data[10];
              snprintf(json_data, 60, "{\"Val\":%d}", received_number);
              Serial.print("received number: ");
              Serial.println(json_data);
              LoRa_send(19, (uint8_t*) json_data, strlen(json_data));
            }
            else
            {
              Serial.println((char*)Lora_receive_buffer);
              LoRa_send(35, Lora_receive_buffer, receiveSize <= sizeof(Lora_receive_buffer) ? receiveSize : sizeof(Lora_receive_buffer), 2);
            }
            time_last_packet = millis();  
            receiveSize = 0;        
        }
          Serial.print("Lora_receive_buffer");
          Serial.println(Lora_receive_buffer[1]+Lora_receive_buffer[0]);
          
         if(Lora_receive_buffer[0]+Lora_receive_buffer[1] == 157 )
         {
          Serial.print("..............................ON");
          digitalWrite(A1,HIGH);
         }
         Serial.print("...............................OFF");
         if(Lora_receive_buffer[0]+Lora_receive_buffer[1] == 149 )
         {
          digitalWrite(A1,LOW);
         }
/*          Serial.print("sending received data");
          Serial.println(received_data);
          if(LoRa_send(35, (uint8_t*)received_data, receiveSize <= sizeof(received_data) ? receiveSize : sizeof(received_data), 2) == true)
          {
            received_data = 0;
          }*/          
    
    status_sched.print_soonest();
    if(Lora_requires_reset())
    {
        badger_restart();        
    }
    badger_sleep_now(s_badger_sleep_period_ms);
}


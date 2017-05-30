* **badger_init()** -  Mandatory to use badgerboard library.

* ****badger_temp_hum_get(float* temp, float* hum)** - Returns onboard temperature and humidity sensor readings. 

**e.g** 

  
      void loop() 
        {
          static float temp = 0;
          static float hum = 0;
          badger_temp_hum_get(&temp,&hum);
          Serial.print("temperature  :  ");
          Serial.println(temp);
          Serial.print("humidity  :  ");
          Serial.println(hum);
          delay(4000);
        }


* **Lora_requires_reset()** - If failed packets is greater than 10(_in a row not total_) this function returns true
**_reccommended to use with badger_restart()_**.




* **LoRa_init(const uint8_t dev_EUI[8], const uint8_t app_EUI[8], const uint8_t app_Key[16])** -  Starts communication with LoRa module
**_devEUI, appkey and appEUI_** have to be previously defined.

**e.g**.

        const uint8_t devEUI[8] = {
        0x87, 0x65, 0x43, 0x23,
        0x45, 0x67, 0x87, 0x65
        };

       const uint8_t appKey[16] = {
        0x76, 0x54, 0x32, 0x34,
        0x56, 0x78, 0x98, 0x76,
        0x54, 0x32, 0x23, 0x45,
        0x67, 0x87, 0x65, 0x43
        };

       const uint8_t appEUI[8] = {
       0x65, 0x43, 0x23, 0x45,
       0x67, 0x88, 0x76, 0x54
       };

        void setup()
        {
            badger_init();
           LoRa_init(devEUI, appEUI, appKey);
        }

* **badger_print_EUI(const uint8_t dev_EUI[8])** - Prints devEUI to serial monitor.

* ***LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len)** - User can use this function to send custom information to custom port.


* ***LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len, int8_t send_count)** - User can use this function to send custom information to custom port ( send_count defines ammount of retries ).


* **badger_temp_sensor_send()** - Sends temperature and humidity to port 19 as **_JSON_**.


* **badger_hum_send()** - Sends onboard humidity sensor readings to port 4.


* **badger_temp_send()** - Sends onboard temperature sensor readings to port 1.


* **LoRa_add_sensitivity(uint8_t sensitivity)**  - User can force LoRa to use different spreading factor
_example_ is under **file &rarr; Examples &rarr; Badgerboard &rarr; LoRa_sf_change**. This example waits for user numeric input to Serial Monitor from **1 to 6** which change spreading factor correspondingly from **7 to 12**.

* **badger_temp_sensor_send_status(uint8_t status)** - Sends temperature, humidity and voltage readings to port 19 as JSON (gives user opportunity to send custom bytes as status).

* **badger_read_vcc_mv()** - Returns **MCU** LDO regulated supply voltage in milliVolts (not actual battery/external power supply voltage), usually 3300 mV. Useful for end of battery detection.

* **badger_restart()** - Restarts Badgerboard.

* **sendCayanneTempHum()** - Send Cayanne standard packet with temperature and humidity .

* **badger_sleep_now(uint32_t period_ms)** - Put badger to sleep for period_ms on **WDT**.

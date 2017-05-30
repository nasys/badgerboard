#include <Arduino.h>
//#include <EEPROM.h>

#include "LowPower_mod.h"
#include "SoftReset_mod.h"

#include "FaBoHumidity_HTS221_mod.h"


#include "badger.h"
#include "Sodaq_RN2483_mod.h"
#include "cayanneLPP.h"

//volatile bool sodaq_wdt_flag = false;

void sodaq_wdt_enable(wdt_period period)
{
	
	// From TPH_Demo/MyWatchdog.cpp
	// Both WDE and WDIE
	__asm__ __volatile__ (  \
						  "in __tmp_reg__,__SREG__" "\n\t"    \
						  "cli" "\n\t"    \
						  "wdr" "\n\t"    \
						  "sts %0,%1" "\n\t"  \
						  "out __SREG__,__tmp_reg__" "\n\t"   \
						  "sts %0,%2" "\n\t" \
						  : /* no outputs */  \
						  : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
        "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
        "r" ((uint8_t) (((period & 0x08) ? _WD_PS3_MASK : 0x00) | \
						_BV(WDE) | _BV(WDIE) | (period & 0x07)) ) \
						  : "r0"  \
						  );
}


// Resets the WDT counter
void sodaq_wdt_reset()
{
	
	// Using avr/wdt.h
	wdt_reset();
	
	// Should this be called once per interrupt,
	// or are we ok calling it with every reset?
	WDTCSR |= _BV(WDIE);	
}

//
void sodaq_wdt_safe_delay(uint32_t ms)
{
	sleep_wdt_approx(ms);
}

static uint64_t millis_offset = 0;

uint32_t badger_millis()
{
	// overflow has to be considered..
	return millis() + millis_offset;
}

void sleep_wdt(period_t period, uint16_t period_ms, uint32_t* remainder)
{
	uint16_t divider = *remainder / period_ms;
	for(uint16_t i = 0; i < divider; i++)
	{
		if(badger_usb_powered())
		{
			delay(period_ms);
		}
		else
		{
			LowPower.powerDown(period, ADC_OFF, BOD_ON);
			millis_offset += period_ms;
		}
		*remainder = *remainder - period_ms;
	}
}

void sleep_wdt_approx(uint32_t sleep_time_ms)
{
	// guarantees no sleep after boot for usb to wake
	if(badger_usb_powered()) // || millis() < 6000)
	{
		delay(sleep_time_ms);
		return;
	}
	// since WDT oscillator accuracy is extremely low, compensating for sleep leftovers (<15ms) makes no sense. 
	if(sleep_time_ms < 60)
	{
		sleep_wdt(SLEEP_30MS, 30, &sleep_time_ms);	
		sleep_wdt(SLEEP_15MS, 15, &sleep_time_ms);
		delay(sleep_time_ms);
		return;
	}
	sleep_wdt(SLEEP_8S, 8000, &sleep_time_ms);
	sleep_wdt(SLEEP_4S, 4000, &sleep_time_ms);
	sleep_wdt(SLEEP_2S, 2000, &sleep_time_ms);
	sleep_wdt(SLEEP_1S, 1000, &sleep_time_ms);
	sleep_wdt(SLEEP_500MS, 500, &sleep_time_ms);
	sleep_wdt(SLEEP_250MS, 250, &sleep_time_ms);  
	sleep_wdt(SLEEP_120MS, 120, &sleep_time_ms);
	sleep_wdt(SLEEP_60MS, 60, &sleep_time_ms);
	sleep_wdt(SLEEP_30MS, 30, &sleep_time_ms);
	sleep_wdt(SLEEP_15MS, 15, &sleep_time_ms);
}


// Boot message
const uint8_t bootMSG[] =
{
	0xFF, 0xFF, 0xFF, 0xAB, 0xBA
};

const uint8_t reOTAA[] =
{
	0xFF, 0xFF, 0xFF, 0xBE, 0xEF
};

static uint32_t s_tx_last_time = 0;
static uint32_t s_tx_interval_min = 30000;

static uint32_t s_tx_successful_count = 0;
static uint32_t s_tx_failed_count = 0;

static bool s_LoRa_down = false;

bool Lora_tx_ready()
{
	return badger_millis() - s_tx_last_time < s_tx_interval_min ? false : true;
}

void Lora_down_check(bool success)
{
	static uint16_t s_failed_packets = 0;
	s_failed_packets++;
	if(success == true)
	{
		s_failed_packets = 0;
		s_LoRa_down = false;
	}
	if(s_failed_packets > 10)
	{
#ifdef	SERIAL_DEBUG	
		Serial.println(F("Too many failed packets. should restart!"));
#endif
		s_LoRa_down = true;
	}
}

void Lora_transmit_counter(bool success)
{
	if(success)
	{
		s_tx_successful_count++;
	}
	else
	{
		s_tx_failed_count++;
	}
	Lora_down_check(success);
}

bool Lora_requires_reset()
{
	return s_LoRa_down;
}

void badger_reset_lora()
{
	pinMode(LORA_RESET_PIN, OUTPUT);
	digitalWrite(LORA_RESET_PIN, LOW);
	sleep_wdt_approx(15);
	digitalWrite(LORA_RESET_PIN, HIGH);
}

bool LoRa_add_sensitivity(uint8_t sensitivity)
{
	LoRaBee.wakeUp();
	LoRaBee.wakeUp();
	sensitivity = sensitivity - 1;					
	if(sensitivity > 6)
		sensitivity = 6;
	sensitivity = 5 - sensitivity;
	Serial.print("Lora: Adding sensitivity: SF ");
	Serial.println(12 - sensitivity);
	bool ok = LoRaBee.setSpreadingFactor(12 - sensitivity);
	ok = LoRaBee.setTXPower(15) && ok;
	ok = LoRaBee.setDataRate(sensitivity) && ok;
	return ok;
	}


void LoRa_init_sleep()
{
	badger_reset_lora();
	loraSerial.begin(DEFAULT_BAUDRATE);
	LoRaBee.init(loraSerial);
	LoRaBee.wakeUp();
	LoRaBee.resetDevice();
	LoRaBee.sleep();
}

//void LoRa_sleep()
//{
//	LoRaBee.sleep();
//}

bool LoRa_init(const uint8_t dev_EUI[8], const uint8_t app_EUI[8], const uint8_t app_Key[16])
{
	LoRaBee.setDiag(Serial);

	if(LoRaBee.initOTA(loraSerial, dev_EUI, app_EUI, app_Key, true) == false)
	{
		badger_reset_lora();
		sleep_wdt_approx(1 * 1000UL);
		if(LoRaBee.initOTA(loraSerial, dev_EUI, app_EUI, app_Key, true) == false)
		{
			badger_restart();
			sleep_wdt_approx(4000);
			return false;
		}
	}

	bool success = false;
	int i = 0;
	while(i <= 4)
	{
		if (LoRaBee.sendOTA() == true)
		{
			LoRaBee.sleep();
			s_tx_last_time = badger_millis();
#ifdef	SERIAL_DEBUG
			Serial.println(F("OTAA successful."));
#endif
			sleep_wdt_approx(12 * 1000UL);
			bool boot_msg_success = LoRa_send(99, bootMSG, 5, 4);
			int u = 0;
			while(!boot_msg_success && u++ < 4)
			{
				while(!Lora_tx_ready())
				{
					sleep_wdt_approx(4 * 1000UL);
					
				}
				
				boot_msg_success = LoRa_resend_try();
			}
			success = boot_msg_success;
			break;
		}
		else
		{
			LoRa_add_sensitivity(i);
			LoRaBee.sleep();
			badger_pulse_led(45);	
			sleep_wdt_approx(60);	  
			badger_pulse_led(45);
			sleep_wdt_approx(10000UL * (i + 1));
			i++;
		}
	}
	if(success == false)
	{
		sleep_wdt_approx(32000);
#ifdef	SERIAL_DEBUG
		Serial.println(F("Booting failed."));
#endif
		badger_restart();
		sleep_wdt_approx(4000);
		return false;
	}
#ifdef	SERIAL_DEBUG
	Serial.println(F("Boot msg sent. "));	
#endif
	s_tx_interval_min = 20000UL * (1 << i);
	badger_pulse_led(10);
	sleep_wdt_approx(60);
	badger_pulse_led(10);
	sleep_wdt_approx(60);
	badger_pulse_led(10);
	sleep_wdt_approx(60);
	badger_pulse_led(10);
	sleep_wdt_approx(60);
	badger_pulse_led(10);
	return success;
}

bool LoRa_initABP(const uint8_t devAddr[4], const uint8_t appSKey[16], const uint8_t nwkSKey[16], bool adr)			
{
	LoRaBee.setDiag(Serial);
	
	if(LoRaBee.initABP(loraSerial, devAddr, appSKey, nwkSKey, false) == false)
	{
		badger_restart();
		sleep_wdt_approx(4000);
		return false;
	}
	LoRaBee.sleep();
	bool boot_msg_success = LoRa_send(99, bootMSG, 5, 4);
	int u = 0;
	while(!boot_msg_success && u++ < 4)
	{
		while(!Lora_tx_ready())
		{
			sleep_wdt_approx(4 * 1000UL);
		}
		boot_msg_success = LoRa_resend_try();
	}
	return boot_msg_success;
}

struct resend_data
{
	uint8_t fPort = 0;
	uint8_t data[LORA_TX_BUF_LEN_MAX];
	uint8_t len = 0;
	uint8_t count = 0;
} s_resend_data;

bool LoRa_resend_store(uint8_t fPort, const uint8_t* data, uint8_t len, int8_t send_count)
{
	if(s_resend_data.count != 0)
	{
#ifdef	SERIAL_DEBUG
		Serial.println(F("Cant take new packet into resend buffer, previous not sent"));
#endif
		return false;
	}
	s_resend_data.fPort = fPort;
	s_resend_data.len = len;
	s_resend_data.count = send_count;
	memcpy(s_resend_data.data, data, len);
#ifdef	SERIAL_DEBUG
	Serial.print(F("Packet inited in resend buffer. Retries "));
	Serial.println(send_count);
	Serial.println((char*)s_resend_data.data);
#endif
	return true;
}

bool LoRa_resend_try()
{
	bool success = false;
	if(s_resend_data.count > 0)
	{
		if(Lora_tx_ready())
		{
			success = LoRa_send(s_resend_data.fPort, s_resend_data.data, s_resend_data.len, 1);
//			Lora_transmit_counter(success);
			if(success)
			{
				s_resend_data.count = 0;
#ifdef	SERIAL_DEBUG
				Serial.println(F("Resend successful"));
#endif
			}
			else
			{
				s_resend_data.count--;
#ifdef	SERIAL_DEBUG
				Serial.print(F("Resend failed, Retries left "));			
				Serial.println(s_resend_data.count);
#endif
			}
		}
		else
		{
#ifdef	SERIAL_DEBUG
			Serial.println(F("Resend Wait."));
#endif
		}
	}
	return success;
}

bool LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len, int8_t send_count)
{
	LoRaBee.wakeUp();
	LoRaBee.wakeUp();

	uint8_t lora_status;
	if(send_count == NO_ACK)
	{
		lora_status = LoRaBee.send(fPort, data, len);
	}
	else
	{
		lora_status = LoRaBee.sendReqAck(fPort, data, len, 2);
	}
	LoRaBee.sleep();
//	Serial.println((char*)data);
	bool success = false;
	switch (lora_status) {
		case NoError:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Successful transmission."));
#endif
			success = true;
			break;
		case NoResponse:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: No response."));
#endif
			break;
		case Timeout:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Timed-out. Check serial."));
#endif
			break;
		case PayloadSizeError:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Payload too large!"));
#endif
			break;
		case InternalError:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Internal error."));
#endif
			badger_restart();
			break;
		case Busy:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: The device is busy."));
#endif
			break;
		case NetworkFatalError:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Network fatal error."));
#endif
			badger_restart();
			break;
		case NotConnected:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: Not connected. Restarting."));
#endif
			badger_restart();
			break;
		case NoAcknowledgment:
#ifdef	SERIAL_DEBUG
			Serial.println(F("Lora: No acknowledgment."));
#endif
			break;
		default:
			break;
	}
	Lora_transmit_counter(success);
	send_count--;
	if(success == false && send_count > 0)
	{
		//LoRa_resend_store(fPort, data, len, send_count);
	}
	s_tx_last_time = badger_millis();
	return success;
}

bool LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len)
{
	return LoRa_send(fPort, data, len, 1); //NO_ACK)
}

uint16_t LoRa_receive(uint8_t* buffer, uint16_t size, uint16_t payloadStartPosition)
{
	return LoRaBee.receive(buffer, size, payloadStartPosition);
}

FaBoHumidity_HTS221 faboHumidity;
static bool s_fabo_init_successful = false;

void badger_temp_sensor_init()
{
	if (faboHumidity.begin()) 
	{
#ifdef	SERIAL_DEBUG
		Serial.println(F("Configured onboard temp/humidity sensor."));
#endif
		s_fabo_init_successful = true;
	}
	else
	{
#ifdef	SERIAL_DEBUG
		Serial.println(F("Error. Onboard temp/hum sensor not responding!"));
#endif
		s_fabo_init_successful = false;
		badger_pulse_led(2);
		sleep_wdt_approx(120);
		badger_pulse_led(2);
	}  
}
void badger_temp_hum_get(float* temp, float* hum)
{
	*temp = faboHumidity.getTemperature();
	*hum = faboHumidity.getHumidity();
	Serial.print("fabo temp");
	Serial.println(*temp);
	Serial.print("fabo hum");
	Serial.println(*hum);
	return;
}
	
bool badger_temp_sensor_send()
{
	if(s_fabo_init_successful == false)
		return false;
	char json_data[60];
	float temp = faboHumidity.getTemperature();
	float humidity = faboHumidity.getHumidity();
	
	int temp_decimal1 = (int)temp;
	temp -= temp_decimal1;
	temp *= 100;
	int temp_decimal2 = (int)temp;
	int hum_decimal1 = (int)humidity;
	humidity -= hum_decimal1;
	humidity *= 100;
	int hum_decimal2 = (int)humidity;
	snprintf(json_data, 60, "{\"T\":%d.%d,\"H\":%d.%d}", temp_decimal1, temp_decimal2, hum_decimal1, hum_decimal2);
#ifdef	SERIAL_DEBUG
	Serial.println(json_data);
#endif
	return LoRa_send(19, (uint8_t*) json_data, strlen(json_data));
}

bool badger_temp_send()
{
	if(s_fabo_init_successful == false)
		return false;
	
	float temp1 = faboHumidity.getTemperature();
	
#ifdef	SERIAL_DEBUG
	Serial.println(temp1);
#endif
	return LoRa_send(1, (uint8_t*) &temp1, sizeof(temp1));
}

bool badger_hum_send()
{
	if(s_fabo_init_successful == false)
		return false;
	
	uint8_t humidity = int(faboHumidity.getHumidity());
	
#ifdef	SERIAL_DEBUG
	Serial.println(humidity);
#endif
	return LoRa_send(4, (uint8_t*) &humidity, sizeof(humidity));
}




bool badger_temp_sensor_send_status(uint8_t status)
{
	char json_data[LORA_TX_BUF_LEN_MAX];
	float temp = 0;
	float humidity = 0;
	if(s_fabo_init_successful == true)
	{
		temp = faboHumidity.getTemperature();
		humidity = faboHumidity.getHumidity();
	}	
	float voltage = badger_read_vcc_mv() / 1000.0;
	
	int temp_decimal1 = (int)temp;
	temp -= temp_decimal1;
	temp *= 10;
	int temp_decimal2 = (int)temp;
	int hum_decimal1 = (int)humidity;
	humidity -= hum_decimal1;
	humidity *= 10;
	int hum_decimal2 = (int)humidity;
	
	int volt_decimal1 = (int)voltage;
	voltage -= volt_decimal1;
	voltage *= 10;
	int volt_decimal2 = (int)voltage;
	Serial.println(json_data);
	snprintf(json_data, LORA_TX_BUF_LEN_MAX, "{\"S\":%d,\"T\":%d.%d,\"H\":%d.%d,\"V\":%d.%d,\"P\":%d/%d}", status, temp_decimal1, temp_decimal2, hum_decimal1, hum_decimal2, volt_decimal1, volt_decimal2, (uint16_t)s_tx_successful_count + 1, (uint16_t)s_tx_failed_count);
#ifdef	SERIAL_DEBUG
	Serial.println(json_data);
#endif
	return LoRa_send(19, (uint8_t*) json_data, strlen(json_data), NO_ACK);
}


bool badger_usb_powered()
{ 
	return USBSTA & (1 << VBUS);
}


void badger_serial_check_connection()
{
	static bool state_last = false;
	bool state_now = badger_usb_powered();
	if(state_now == true)
	{  //checks state of VBUS
		if(!Serial)
		{
			Serial.begin(57600);
		}
		if(state_now != state_last)
		{
			pinMode(LED_PIN, OUTPUT);
			badger_pulse_led(100);
		}
	}
	else
	{
		if(Serial)
		{
			Serial.end();
			pinMode(LED_PIN, INPUT);		
		}

	}
	state_last = state_now;
}


void badger_pulse_led(uint16_t period_ms)
{
	digitalWrite(LED_PIN, HIGH);
	sleep_wdt_approx(period_ms);
	digitalWrite(LED_PIN, LOW);	
}

bool badger_blink_error(bool success)
{
	if(!success)
	{
		badger_pulse_led(2);
		sleep_wdt_approx(128);
		badger_pulse_led(2);
		sleep_wdt_approx(128);
		badger_pulse_led(2);
		sleep_wdt_approx(128);
		badger_pulse_led(2);
		sleep_wdt_approx(128);
		badger_pulse_led(2);
	}
	return !success;
}

void badger_print_EUI(const uint8_t dev_EUI[8])
{
	Serial.print("devEUI ");
	Serial.print(dev_EUI[0], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[1], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[2], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[3], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[4], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[5], HEX);
	Serial.print(" ");
	Serial.print(dev_EUI[6], HEX);
	Serial.print(" ");
	Serial.println(dev_EUI[7], HEX);

	
}


long badger_read_vcc_mv() 
{ 
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  
	
	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA,ADSC)); // measuring
	
	uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both
	
	long result = (high<<8) | low;
	
	result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}

uint32_t min_u32(uint32_t x, uint32_t y)
{
	if(x < y)
		return x;
	else
		return y;
}

uint32_t badger_scheduler::next_in = 0xFFFFFF;
bool badger_scheduler::packet_tarried = false;

badger_scheduler::badger_scheduler(uint32_t period_ms, uint32_t start_offset_ms, uint8_t mode)
{
	time_period = period_ms;
	first_time_run = true;
	if(start_offset_ms == 0)
	{
		start_offset_enabled = false;
		time_start_offset = 0;
	}
	else
	{
		start_offset_enabled = true;
		time_start_offset = start_offset_ms;
	}
	next_in = 0xFFFFFF;
	tx_ignore = false;
	if(mode == PRINT_ENABLE)
		printing_enable = true;
	else if(mode == PRINT_DISABLE)
		printing_enable = false;
	else if(mode == OTHER_SCHED)
	{
		tx_ignore = true;
		printing_enable = false;
	}
}

bool badger_scheduler::run_now()
{
	uint32_t time_now = badger_millis();
	bool success = false;
	// run only once
	if(first_time_run == true)
	{
		if(printing_enable)
		{
			next_in = min_u32(time_start_offset, next_in);
		}
		time_start_offset += badger_millis();
		first_time_run = false;
	}		
	// run only up until the offset is completed
	if(start_offset_enabled == true)
	{
		// start offset not yet passed
		if(time_now < time_start_offset)
		{
			if(printing_enable)
			{
				next_in = min_u32(time_start_offset - time_now, next_in);
			}
			return false;
		}
		// start offset passedsend the packet
		else if(tx_ignore || Lora_tx_ready())
		{
			start_offset_enabled = false;
			time_last = time_now;
			success = true;
		}
		// would send, but cant, sending postponed
		else if(!tx_ignore)
		{
			if(printing_enable)
			{
				packet_tarried = true;
			}
		}
	}
	// normal mode
	else
	{
		// time to send the packet
		if(time_now - time_last >= time_period)
		{
			if(tx_ignore || Lora_tx_ready())
			{
				time_last = time_now;
				success = true;
			}
			else if(!tx_ignore)
			{
				if(printing_enable)
				{
					// packet tarried
					packet_tarried = true;
				}
			}
		}
	}
	if(printing_enable)
	{
		next_in = min_u32(time_period - (time_now - time_last), next_in);
	}
	return success;
}

void badger_scheduler::print_soonest()
{
	if(packet_tarried == false)
	{
		Serial.print("next TX in ");
		Serial.print((next_in + 500) / 1000);
		Serial.print("s P ");
		Serial.print(s_tx_successful_count);	
		Serial.print("/");		
		Serial.println(s_tx_failed_count);			
	}
	else
	{
		packet_tarried = false;
		Serial.println("TX postponed.");
	}
	next_in = 0xFFFFFFFF;
}

void badger_init()
{
    LoRa_init_sleep();
    pinMode(LED_PIN, INPUT);
    badger_serial_check_connection();
    badger_pulse_led(2000);
	badger_temp_sensor_init();
}

badger_scheduler rare_sched(2000UL, 1, OTHER_SCHED);



bool sendCayanneTempHum()			
{
	
	if(s_fabo_init_successful == false)
	{
		return false;
	}
	float temperature = faboHumidity.getTemperature();
	float relativeHumidity = faboHumidity.getHumidity();

	CayenneLPP cayennePacket(10);
	cayennePacket.addTemperature(1 , temperature);
	cayennePacket.addRelativeHumidity(1, relativeHumidity);
	
	LoRa_send(69,cayennePacket.getBuffer(),cayennePacket.getSize());
}

bool badger_sleep_now(uint32_t period_ms)
{
	bool rare = rare_sched.run_now() ? true : false;
	bool success = false;
	if(rare)
	{
		success = LoRa_resend_try();
	}
	sleep_wdt_approx(period_ms);
    if(rare)
	{
		badger_serial_check_connection();
    }
	return success;
}

void badger_restart()
{
#ifdef	SERIAL_DEBUG
	Serial.println("Badger attempting to restart");
#endif
	delay(30);
	soft_restart();
}

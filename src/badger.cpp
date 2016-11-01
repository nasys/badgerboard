#include <Arduino.h>
#include <EEPROM.h>

#include "LowPower.h"
#include "SoftReset.h"

#include "FaBoHumidity_HTS221.h"

#include "badger.h"
#include "Sodaq_RN2483.h"

static uint64_t millis_offset = 0;
//static float s_wdt_calib = 1.0;

uint32_t badger_millis()
{
	// overflow has to be considered..
	return millis() + millis_offset;
}

void sleep_wdt(period_t period, uint16_t period_ms, uint32_t* remainder)
{
	uint16_t divider = *remainder / period_ms;
	//  Serial.print(period_ms); Serial.print(" : "); Serial.print(divider); Serial.print(" : "); Serial.print(*remainder);
	for(int i = 0; i < divider; i++)
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
	}
	//  Serial.print(" : "); Serial.println((uint32_t) millis_offset);
	*remainder = *remainder - (divider * period_ms);
}

void sleep_wdt_approx(uint32_t sleep_time_ms)
{
//	delay(sleep_time_ms);
//	return;
	// guarantees no sleep after boot for usb to wake
	if(badger_usb_powered()) // || millis() < 6000)
	{
		delay(sleep_time_ms);
		return;
	}
	if(sleep_time_ms < 15)
	{
		delay(sleep_time_ms);
		//		Serial.println("sleep < 15");
		return;	
	}
	//	sleep_time_ms *= s_wdt_calib;
	if(sleep_time_ms < 28)
	{
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
static bool s_long_range_enabled = false;

static bool s_LoRa_down = false;

void LoRa_tx_interval_min_set(uint32_t interval)
{
	if(interval < 30)
		Serial.println(F("Warning: LoRa TX interval should be higher, eg 30 sec"));
	s_tx_interval_min = interval;
}

bool Lora_tx_ready()
{
	return badger_millis() - s_tx_last_time < s_tx_interval_min ? false : true;
}

void Lora_transmit_counter(bool success)
{
	if(success)
	{
//		Serial.println("success++");		
		s_tx_successful_count++;
	}
	else
	{
//		Serial.println("failed++");
		s_tx_failed_count++;
	}
	Lora_down_check(success);
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
		Serial.println(F("Too many failed packets. should restart!"));
		s_LoRa_down = true;
	}
}

bool Lora_requires_reset()
{
	return s_LoRa_down;
}

bool LoRa_enable_long_range()
{
	LoRaBee.wakeUp();
	bool ok = LoRaBee.setSpreadingFactor(12);
	ok = LoRaBee.setTXPower(15) && ok;
	ok = LoRaBee.setDataRate(1) && ok;
	s_long_range_enabled = true;
	return ok;
}

bool LoRa_init_sleep()
{
	badger_reset_lora();
	loraSerial.begin(DEFAULT_BAUDRATE);
	LoRaBee.init(loraSerial);
	LoRaBee.wakeUp();
	LoRaBee.resetDevice();
	LoRaBee.sleep();
}

void LoRa_sleep()
{
	LoRaBee.sleep();
}

bool LoRa_init(const uint8_t dev_EUI[8], const uint8_t app_EUI[8], const uint8_t app_Key[16])
{
	LoRaBee.setDiag(Serial);
	
	bool success = false;
	int i = 0;
	while(i++ < 5)
	{
		if (LoRaBee.initOTA(loraSerial, dev_EUI, app_EUI, app_Key, true))
		{
			LoRaBee.sleep();
			s_tx_last_time = badger_millis();
			Serial.println(F("OTAA successful."));
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
			LoRaBee.sleep();
			Serial.print(i);
			Serial.println(F(" times Connection failed..retrying."));
			badger_pulse_led(45);	
			sleep_wdt_approx(60);	  
			badger_pulse_led(45);
			sleep_wdt_approx(8000);
			if(i == 2)
			{
				badger_reset_lora();
//				LoRa_enable_long_range();
				Serial.println(F("Resetting LoRa module"));
			}
		}
	}
	if(success == false)
	{
		sleep_wdt_approx(32000);
		Serial.println(F("Booting failed."));
		badger_restart();
		delay(4000);
		return;
	}
	Serial.println(F("Boot msg sent. "));	
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

struct resend_data
{
	uint8_t fPort = 0;
	uint8_t data[LORA_TX_BUF_LEN_MAX];
	uint8_t len = 0;
	uint8_t count = 0;
} s_resend_data;

bool LoRa_resend_store(uint8_t fPort, uint8_t* data, uint8_t len, int8_t send_count)
{
	if(s_resend_data.count != 0)
	{
		Serial.println(F("Cant take new packet into resend buffer, previous not sent"));
		return false;
	}
	s_resend_data.fPort = fPort;
	s_resend_data.len = len;
	s_resend_data.count = send_count;
	memcpy(s_resend_data.data, data, len);
	Serial.println(F("Packet inited in resend buffer"));
	Serial.println((char*)s_resend_data.data);
	Serial.println(send_count);
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
				Serial.println(F("resend successful"));
			}
			else
			{
				s_resend_data.count--;
				Serial.print(F("resend failed, left "));			
				Serial.println(s_resend_data.count);
			}
		}
		else
		{
			Serial.println(F("Resend Wait."));
		}
	}
	return success;
}

bool LoRa_send(uint8_t fPort, uint8_t* data, uint8_t len, int8_t send_count)
{
	LoRaBee.wakeUp();
	LoRaBee.wakeUp();
//	delay(1000);
	if(s_long_range_enabled == true)
		LoRaBee.setDataRate(1);

	uint8_t lora_status;
	if(send_count == NO_ACK)
	{
		lora_status = LoRaBee.send(fPort, data, len);
	}
	else
	{
		lora_status = LoRaBee.sendReqAck(fPort, data, len, 1);
	}
//	delay(1000);
	LoRaBee.sleep();
//	Serial.println((char*)data);
	bool success = false;
	switch (lora_status) {
		case NoError:
			Serial.println(F("Successful transmission."));
			success = true;
			break;
		case NoResponse:
			Serial.println(F("No response."));
			break;
		case Timeout:
			Serial.println(F("Timed-out. Check serial."));
			break;
		case PayloadSizeError:
			Serial.println(F("Payload too large!"));
			break;
		case InternalError:
			Serial.println(F("Internal error."));
			badger_restart();
			break;
		case Busy:
			Serial.println(F("The device is busy."));
			break;
		case NetworkFatalError:
			Serial.println(F("Network fatal error."));
			badger_restart();
			break;
		case NotConnected:
			Serial.println(F("Not connected. Restarting."));
			badger_restart();
			break;
		case NoAcknowledgment:
			Serial.println(F("No acknowledgment."));
			break;
		default:
			break;
	}
	Lora_transmit_counter(success);
	send_count--;
	if(success == false && send_count > 0)
	{
		LoRa_resend_store(fPort, data, len, send_count);
	}
	s_tx_last_time = badger_millis();
	return success;
}

bool LoRa_send(uint8_t fPort, uint8_t* data, uint8_t len)
{
	return LoRa_send(fPort, data, len, 1); //NO_ACK);
}

/*void LoRa_sleep()
{
	LoRaBee.wakeUp();
	LoRaBee.sleep();
}*/

FaBoHumidity_HTS221 faboHumidity;
static bool s_fabo_init_successful = false;

void badger_temp_sensor_init()
{
	if (faboHumidity.begin()) {
		Serial.println(F("configured onboard temp/humidity sensor."));
		s_fabo_init_successful = true;
	}
	else
	{
		Serial.println(F("Onboard temp/hum sensor not responding"));
		s_fabo_init_successful = false;
		badger_pulse_led(2);
		sleep_wdt_approx(120);
		badger_pulse_led(2);
	}  
}

bool badger_temp_sensor_send()
{
	if(s_fabo_init_successful == false)
	{
		return false;
	}
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
	Serial.println(json_data);
	return LoRa_send(19, (uint8_t*) json_data, strlen(json_data));
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
	
	snprintf(json_data, LORA_TX_BUF_LEN_MAX, "{\"S\":%d,\"T\":%d.%d,\"H\":%d.%d,\"V\":%d.%d,\"P\":%d/%d}", status, temp_decimal1, temp_decimal2, hum_decimal1, hum_decimal2, volt_decimal1, volt_decimal2, (uint16_t)s_tx_successful_count + 1, (uint16_t)s_tx_failed_count);
	Serial.println(json_data);
	return LoRa_send(19, (uint8_t*) json_data, strlen(json_data), NO_ACK);
}



bool badger_usb_check_init()
{
	USBCON |= (1 << OTGPADE);
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
	Serial.print(dev_EUI[6], HEX);
	Serial.print(" ");
	Serial.println(dev_EUI[7], HEX);
}

void badger_reset_lora()
{
	pinMode(LORA_RESET_PIN, OUTPUT);
	digitalWrite(LORA_RESET_PIN, LOW);
	sleep_wdt_approx(15);
	digitalWrite(LORA_RESET_PIN, HIGH);
}



uint64_t EEP_read_u64(uint16_t addr)
{
	union
	{
		byte b[8];
		uint64_t d;
	} data;
	for(int i = 0; i < 8; i++)
	{
		data.b[i] = EEPROM.read(addr+i);
	}
	return data.d;
}

void EEP_write_u64(uint16_t addr, uint64_t x)
{
	union
	{
		byte b[8];
		uint64_t d;
	} data;
	data.d = x;
	for(int i = 0; i < 8; i++)
	{
		EEPROM.write(addr+i, data.b[i]);
	}
}

static uint64_t s_last_stored_value = 0;

uint64_t EEP_load_u64_counter()
{
	if(EEPROM.read(EEP_COUNTER_CHECK_ADDR) == EEP_COUNTER_CHECK_VALUE)
	{
		uint64_t value = EEP_read_u64(EEP_COUNTER_DATA_ADDR);
		s_last_stored_value = value;
		Serial.print("Restored from EEP ");Serial.println((uint32_t)value);
		return value;
	}
	else
	{
		EEP_write_u64(EEP_COUNTER_DATA_ADDR, 0);
		EEPROM.write(EEP_COUNTER_CHECK_ADDR, EEP_COUNTER_CHECK_VALUE);
		Serial.println("First init. Stored to EEP 0");
		return 0;
	}
}

void EEP_store_u64_counter(uint64_t x, uint16_t threshold_count)
{
	if(x - s_last_stored_value > threshold_count)
	{
		EEP_write_u64(EEP_COUNTER_DATA_ADDR, x);
		s_last_stored_value = x;
		Serial.print("EEP renewed ");Serial.println((uint32_t)x);
	}
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

bool badger_init()
{
    LoRa_init_sleep();
    pinMode(LED_PIN, INPUT);
    badger_serial_check_connection();
    badger_pulse_led(2000);
    badger_temp_sensor_init();
}

badger_scheduler rare_sched(2000UL, 1, OTHER_SCHED);

bool badger_sleep_now(uint32_t period_ms)
{
	bool rare = rare_sched.run_now() ? true : false;
	bool success;
	if(rare)
	{
		success = LoRa_resend_try();
	}
	if(badger_usb_powered() && period_ms > 4 * 1000UL)
	{
		period_ms = 4 * 1000UL;
	}
	sleep_wdt_approx(period_ms);
    if(rare)
	{
		badger_serial_check_connection();
    }
//	badger_pulse_led(0);
	return true;
}

void badger_restart()
{
	Serial.println("Badger attempting to restart");
	delay(30);
	soft_restart();
}

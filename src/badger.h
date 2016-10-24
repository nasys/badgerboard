#ifndef SODAQ_WDT_H_
#define SODAQ_WDT_H_

#ifdef ARDUINO_ARCH_AVR

#include <avr/wdt.h>
#include <Arduino.h>


#endif

#define loraSerial Serial1

#define LED_PIN			13
#define LORA_RESET_PIN	5
//#define LORA_PACKET_PERIOD_MIN_MS	6000

#define LED_FEEDBACK

#define EEP_COUNTER_CHECK_VALUE	0xA5
#define EEP_COUNTER_CHECK_ADDR	10
#define EEP_COUNTER_DATA_ADDR	EEP_COUNTER_CHECK_ADDR + 1

#define LORA_TX_BUF_LEN_MAX		50

// Approximate periods supported by both platforms
// The SAMD also supports 8ms and 16s.
enum wdt_period : uint8_t 
{

  // See avr/wdt.h
  WDT_PERIOD_1DIV64  = WDTO_15MS,  // 15ms   = ~1/64s
  WDT_PERIOD_1DIV32  = WDTO_30MS,  // 30ms   = ~1/32s
  WDT_PERIOD_1DIV16  = WDTO_60MS,  // 60ms   = ~1/16s
  WDT_PERIOD_1DIV8   = WDTO_120MS, // 120ms  = ~1/8s
  WDT_PERIOD_1DIV4   = WDTO_250MS, // 250ms  = 1/4s
  WDT_PERIOD_1DIV2   = WDTO_500MS, // 500ms  = 1/2s
  WDT_PERIOD_1X      = WDTO_1S,    // 1000ms = 1s
  WDT_PERIOD_2X      = WDTO_2S,    // 2000ms = 2s
  WDT_PERIOD_4X      = WDTO_4S,    // 4000ms = 4s
  WDT_PERIOD_8X      = WDTO_8S     // 8000ms = 8s
  
};

void sodaq_wdt_enable(wdt_period period = WDT_PERIOD_1X);

void sodaq_wdt_disable();

void sodaq_wdt_reset();

void sodaq_wdt_safe_delay(uint32_t ms);

extern volatile bool sodaq_wdt_flag;

void calib_wdt_timer();

void sleep_wdt_approx(uint32_t sleep_time_ms);

uint32_t badger_millis();

void LoRa_tx_interval_min_set(uint32_t interval);

bool Lora_tx_ready();

bool Lora_requires_reset();

bool LoRa_enable_long_range();

bool LoRa_init_sleep();

bool LoRa_init(const uint8_t dev_EUI[8], const uint8_t app_EUI[8], const uint8_t app_Key[16]);

bool LoRa_send(uint8_t fPort, uint8_t* data, uint8_t len);

bool LoRa_send(uint8_t fPort, uint8_t* data, uint8_t len, int8_t send_count);

void Lora_down_check(bool success);

bool LoRa_resend_try();

void LoRa_sleep();

void badger_temp_sensor_init();

bool badger_temp_sensor_send();

bool badger_temp_sensor_send_status(uint8_t status);

void badger_pulse_led(uint16_t period_ms);

bool badger_blink_error(bool success);

void badger_print_EUI(const uint8_t dev_EUI[8]);

void badger_reset_lora();

uint64_t EEP_load_u64_counter();

void EEP_store_u64_counter(uint64_t x, uint16_t threshold_count);

long badger_read_vcc_mv();

void badger_serial_check_connection();

bool badger_usb_powered();

void badger_restart();

#define PRINT_ENABLE	1
#define PRINT_DISABLE	0

#define OTHER_SCHED		3

#define NO_ACK			0

class badger_scheduler
{
	public:
		badger_scheduler(uint32_t period_ms, uint32_t start_offset_ms, uint8_t mode);
		bool run_now();
		void print_soonest();
		static uint32_t next_in;
		static bool packet_tarried;
	private:
		uint32_t time_period;
		uint32_t time_last;
		uint32_t time_start_offset;
		bool start_offset_enabled;
		bool first_time_run;
		bool printing_enable;
		bool tx_ignore;
};

bool badger_init();

bool badger_sleep_now(uint32_t period_ms);



#endif /* SODAQ_WDT_H_ */




#ifndef SODAQ_WDT_H_
#define SODAQ_WDT_H_

#include <avr/wdt.h>

// comment SERIAL_DEBUG out to get rid of debug messages. sched.print_soonest() will still work, since it is controllable from main file.
#define SERIAL_DEBUG

#define loraSerial Serial1

#define LED_PIN			13
#define LORA_RESET_PIN	5
//#define LORA_PACKET_PERIOD_MIN_MS	6000

#define LED_FEEDBACK

/*
#define EEP_COUNTER_CHECK_VALUE	0xA5
#define EEP_COUNTER_CHECK_ADDR	10
#define EEP_COUNTER_DATA_ADDR	EEP_COUNTER_CHECK_ADDR + 1
#define EEP_COUNTER_DATA_ADDR_2	EEP_COUNTER_DATA_ADDR + 8
*/

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

extern volatile bool sodaq_wdt_flag;
void badger_temp_hum_get(float* temp, float* hum);
//void sodaq_wdt_enable(wdt_period period = WDT_PERIOD_1X);

//void sodaq_wdt_disable();

void sodaq_wdt_reset();

//void sodaq_wdt_safe_delay(uint32_t ms);


/** @brief sleeps on internal watchdog. its not very precise since watchdog timer isn't very precise. */
void sleep_wdt_approx(uint32_t sleep_time_ms);

/** @brief returns milliseconds from the wakeup. Like uptime. Overflows in 49 days. */
uint32_t badger_millis();

/** @brief returns true if minium timeout between two packets have passed. */
bool Lora_tx_ready();

bool Lora_requires_reset();

/** @brief LoRa module wakes up not sleeping deep but consuming hefty 2,8mA. If you don't do the LoRa_init() anytime soon, then put LoRa module to sleep to conserve power. */
void LoRa_init_sleep();

/** @brief Sets up the lora module to send with correct address and appkey etc. LoRa_sleep() doesn't spoil it. */
bool LoRa_init(const uint8_t dev_EUI[8], const uint8_t app_EUI[8], const uint8_t app_Key[16],bool adr);

bool LoRa_initABP(const uint8_t devAddr[4], const uint8_t appSKey[16], const uint8_t nwkSKey[16], bool adr);  //abpinit

/** @brief Sends data over LoRa network into fPort port as a broadcast message. */
bool LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len);

/** @brief Sends data over LoRa network into fPort port and requests acknowledge, if send count > 1. */
bool LoRa_send(uint8_t fPort, const uint8_t* data, uint8_t len, int8_t send_count);

uint16_t LoRa_receive(uint8_t* buffer, uint16_t size, uint16_t payloadStartPosition = 0);

bool LoRa_resend_try();

/** @brief Puts LoRa module into deep sleep mode (saves 2,8mA). */
//void LoRa_sleep();

/** @brief Initalizes the onboard temperature sensor. */
//void badger_temp_sensor_init();

/** @brief Send only temperature and humidity from onboard sensor
	expects temp sensor to be inited. badger_temp_sensor_init() */
bool badger_temp_sensor_send();
bool badger_hum_send();
bool badger_temp_send();


/** @brief Send status message (8bits of data) with temperature, humidity, battery voltage and successful
	unsuccessful packet count in JSON format. */
bool badger_temp_sensor_send_status(uint8_t status);

/** @brief Convenient way to pulse led. */
void badger_pulse_led(uint16_t period_ms);

bool badger_blink_error(bool success);

void badger_print_EUI(const uint8_t dev_EUI[8]); 

//uint64_t EEP_load_u64_counter();

//void EEP_store_u64_counter(uint64_t x, uint16_t threshold_count);

/** @brief Returns the board voltage. If the supply is higher than 3,3V LDO, then still returns 3,3V. 
	Gives usable information only when battery is almost empty and voltage drops fast. */
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
/** @brief cheduler init. creates instance. badger_scheduler(how_often_ms, beginning_offset_ms, if_prints_with_print_soonest). If scheduler is for lora things, then use PRINT_ENABLE or PRINT_DISABLE, if just need general scheduler, use OTHER_SCHED. */
		badger_scheduler(uint32_t period_ms, uint32_t start_offset_ms, uint8_t mode);
/** @brief If run_now() is true, then its time to do whatever was scheduled */
		bool run_now();
/** @brief Prints the soonest event into serial port. init mode parameter changes if current scheduler needs printing. this function should be called only once from any of the insctances. (like static)	*/
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
bool sendCayanneTempHum();
/** @brief Inits LoRa module to sleep, temperature sensor, serial port and finally blinks user led. */
void badger_init();

/** @brief Put badger to sleep for period_ms on wdt. If usb is connected, then does delay inside. */
bool badger_sleep_now(uint32_t period_ms);



#endif /* SODAQ_WDT_H_ */




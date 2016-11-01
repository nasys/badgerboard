

enum modes{
	still_mode = 0,
	normal_mode = 1,
	fast_mode = 2
};

class wmr
{
	public:
	wmr(uint16_t noise_rms, uint8_t ir_pin, uint8_t an_pin, uint8_t trans_pin);
	uint32_t process();
	uint64_t pulse_counter_get();
	
	private:
	
	uint8_t ir_led_pin;
	uint8_t analog_in;
	uint8_t trans_pull;

	
	bool update_min_max(uint16_t value);
	uint16_t measure_reflectance();
	void switch_led(bool led_on);
	uint16_t analog_read();

	uint16_t noise_range;

	uint16_t calibrated_signal_min;
	uint16_t calibrated_signal_max;

	uint16_t led_inpulses_during_half_pulse;
	uint32_t sample_still_delay_ms;
	uint16_t sample_fast_delay_ms;
	uint64_t pulses_counted;

//	uint16_t reflected_last;
	bool pulse_high;
	uint8_t sensor_mode;

	uint32_t sensor_mode_change_time_last;  // ebavajalik??
	uint16_t sensor_value_at_mode_change;
	
	uint16_t samples_since_last_delay_update;	

	uint32_t samples_since_last_pulse;
	uint32_t pulse_time_last;
	uint16_t sampling_delay_ms;
		
//	uint32_t time_since_last_pulse = 0;
//	uint16_t samples_in_previous_pulse;

	uint16_t pulses_in_fast_mode;

	uint16_t reflected_max_last;
	uint16_t reflected_min_last;
	
	uint64_t flow_rate_pulses;
	uint32_t flow_rate_time;

	uint64_t flow_rate_pulses_latest;
	uint32_t flow_rate_time_latest;
};


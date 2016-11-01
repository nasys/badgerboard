#include <Arduino.h>
#include <badger.h>
#include "WMR.h"

const int PULSE_PIN = 13;



uint64_t wmr::pulse_counter_get()
{
	return pulses_counted;
}


bool wmr::update_min_max(uint16_t value)
{
/*    static bool init = true;
    if(init == true)
    {
       calibrated_signal_min = value;
       calibrated_signal_max = value;
	   init = false;
       return false; 
    }*/
    if(value > calibrated_signal_max)
    {
        calibrated_signal_max = value;
        return true;
    }
    if(value < calibrated_signal_min)
    {
       calibrated_signal_min = value;
       return true; 
    }
    return false;
}

uint16_t wmr::measure_reflectance()
{
	digitalWrite(trans_pull, LOW);
	delayMicroseconds(15);
	int dark_val = analog_read();
	switch_led(true);  
	// from led rise to proper analog rise 20us /fall 10us
	delayMicroseconds(25);
	int bright_val = analog_read();
	digitalWrite(trans_pull, HIGH);  
	switch_led(false);
	if(bright_val < dark_val)
		return 0;
	uint16_t reflected = bright_val - dark_val;
	return reflected;
}

void wmr::switch_led(bool led_on)
{
	if(led_on)
		digitalWrite(ir_led_pin, false);
	else
		digitalWrite(ir_led_pin, true);
}


uint16_t wmr::analog_read()
{
  uint16_t result = 0;
  for(int i = 0; i < 1 ; i++)
    result += analogRead(analog_in);
  return result / 1;
}

// noise_rms = 18
wmr::wmr(uint16_t noise_rms, uint8_t ir_pin, uint8_t an_pin, uint8_t trans_pin){
	noise_range = noise_rms;
	ir_led_pin = ir_pin;
	analog_in = an_pin;
	trans_pull = trans_pin;
	
	led_inpulses_during_half_pulse = 4;
	sample_still_delay_ms = 1000 / 5;
	sample_fast_delay_ms = 1000 / 200;
	
    pinMode(ir_led_pin, OUTPUT);
    pinMode(trans_pull, OUTPUT);
    pinMode(PULSE_PIN, OUTPUT);
    pinMode(analog_in, INPUT);    
    digitalWrite(trans_pull, HIGH);

    pulses_counted = EEP_load_u64_counter();

    uint16_t reflected = measure_reflectance();
//	reflected_last = reflected;
	pulse_high = true;
	sensor_mode = still_mode;
	
	uint32_t time_now = badger_millis();	
	sensor_mode_change_time_last = time_now;
	sensor_value_at_mode_change = reflected;
	samples_since_last_delay_update = 0;
	samples_since_last_pulse = 0;
	pulse_time_last = time_now;
	sampling_delay_ms = sample_still_delay_ms;
//	samples_in_previous_pulse = 1;
	pulses_in_fast_mode = 0;
	reflected_max_last = reflected;
	reflected_min_last = reflected;
	
	calibrated_signal_min = reflected;
	calibrated_signal_max = reflected;	
	
	flow_rate_pulses = 0;
	flow_rate_time = time_now;
	flow_rate_pulses_latest = 0;
	flow_rate_time_latest = time_now;
}


uint32_t wmr::process(){
	int16_t reflected = measure_reflectance();
	uint32_t time_now = badger_millis();

	// if still and change greater than noise -> fast mode
	if(sensor_mode == still_mode)
	{
		if(reflected > (sensor_value_at_mode_change + noise_range) || 
		reflected < (sensor_value_at_mode_change - noise_range))
		{
			sensor_mode = fast_mode;
			sensor_mode_change_time_last = time_now;
			sensor_value_at_mode_change = reflected;
			pulses_in_fast_mode = 0;
			// vaja?
//?			time_since_last_pulse = time_now;
			sampling_delay_ms = sample_fast_delay_ms;
//			Serial.println("!!!!rapid - from still to fast!");
			// vaja?
			samples_since_last_pulse = 0;
//			pulse_time_last = time_now;
			flow_rate_pulses = pulses_counted;
			flow_rate_time = time_now;
		}
	}

	// if in fast mode and changes are low - > normal mode
	if(sensor_mode == fast_mode)
	{
		if(samples_since_last_pulse >= 14 || pulses_in_fast_mode > 4)
		{
			sensor_mode = normal_mode;
			sensor_mode_change_time_last = time_now;
			sensor_value_at_mode_change = reflected;
//			Serial.println("from fast to normal mode");
			sampling_delay_ms = sample_fast_delay_ms * 4;
			samples_since_last_delay_update = 0;
		}
	}
	
	if(pulse_high == true)
	{
		if(reflected_min_last > reflected)
		{
			reflected_min_last = reflected;
		}
		if(reflected > (reflected_min_last + noise_range))
		{
			reflected_max_last = reflected;
			pulse_high = false;
			digitalWrite(PULSE_PIN, HIGH);
            delay(1);
            digitalWrite(PULSE_PIN, LOW);
			uint16_t samples_in_previous_pulse = samples_since_last_pulse;
			samples_since_last_pulse = 0;
			uint32_t time_since_last_pulse = time_now - pulse_time_last;
			pulse_time_last = time_now;
			if(sensor_mode == still_mode)
			{
				sensor_mode = fast_mode;
				pulses_in_fast_mode = 0;
				sensor_mode_change_time_last = time_now;
				sensor_value_at_mode_change = reflected;
			}
			if(sensor_mode == fast_mode)
			{
				pulses_in_fast_mode++;
			}
			else
			{
				sampling_delay_ms = (sampling_delay_ms + (time_since_last_pulse / led_inpulses_during_half_pulse)) / 2;
//				if(samples_in_previous_pulse == 0)
//					Serial.println("Errrrrorrrrr, samples since last pulse == 0!!");
				if(samples_in_previous_pulse < 2)
				{
					sampling_delay_ms /= 2;
				}
				samples_since_last_delay_update = 0;
			}
//			Serial.print(samples_in_previous_pulse);
//			Serial.print(" pulse low ");
//			Serial.println(time_since_last_pulse);
		}
	}
	if(pulse_high == false)
	{
		if(reflected_max_last < reflected)
		{
			reflected_max_last = reflected;
		}
		if(reflected < (reflected_max_last - noise_range))
		{
			reflected_min_last = reflected;
			pulse_high = true;
            digitalWrite(PULSE_PIN, HIGH);
            delay(1);
            digitalWrite(PULSE_PIN, LOW);
            		
			uint16_t samples_in_previous_pulse = samples_since_last_pulse;
			samples_since_last_pulse = 0;
			uint32_t time_since_last_pulse = time_now - pulse_time_last;
			pulse_time_last = time_now;
			pulses_counted++;			
			flow_rate_pulses_latest = pulses_counted;
			flow_rate_time_latest = time_now;

			
			if(sensor_mode == still_mode)
			{
				sensor_mode = fast_mode;
				pulses_in_fast_mode = 0;
				sensor_mode_change_time_last = time_now;
				sensor_value_at_mode_change = reflected;
			}		
			if(sensor_mode == fast_mode)
			{
				pulses_in_fast_mode++;
			}
			else
			{
				sampling_delay_ms = (sampling_delay_ms + (time_since_last_pulse / led_inpulses_during_half_pulse)) / 2;
//				if(samples_in_previous_pulse == 0)
//					Serial.println("Errrrrorrrrr, samples since last pulse == 0!!");
				if(samples_in_previous_pulse < 2)
				{
					sampling_delay_ms /= 2;
				}
				samples_since_last_delay_update = 0;
			}
//			Serial.print(samples_in_previous_pulse);
//			Serial.print(" pulse high ");
//			Serial.println(time_since_last_pulse);
			Serial.print("Counted pulses: ");
			Serial.println((uint32_t)pulses_counted );
		}	
	}

	// if in normal mode and no activity for 2 secs
	if(sensor_mode == normal_mode)
	{
		if(time_now - pulse_time_last > 2000)
		{
			sensor_mode = still_mode;
			sensor_mode_change_time_last = time_now;
			sensor_value_at_mode_change = reflected;
//			Serial.println("Standstill");
			sampling_delay_ms = sample_still_delay_ms;
			samples_since_last_delay_update = 0;
		}
		// vaja?
		if(samples_since_last_delay_update > 6){
			samples_since_last_delay_update = 0;
			sampling_delay_ms = (sampling_delay_ms + (time_now - pulse_time_last)) / 2;
		}

	}


	// pulse has changed in this loop
//	if(samples_in_previous_pulse > 0)
//	{
		// make sure new timing is calculated
//		samples_since_last_delay_update = 100;
//	}

	if(pulses_counted < 20)
	{
		update_min_max(reflected);
	}
	else if(pulses_counted == 20)
	{
		calibrated_signal_min -= noise_range;
		calibrated_signal_max += noise_range;
		// rats
		pulses_counted++;
		Serial.println("Calibration finished!!");
	}
	else
	{
		if(reflected > calibrated_signal_max || reflected < calibrated_signal_min)
		{
//			Serial.println("    EEEERRRRROOOOOORRRR! Someone has moved the sensor! Possibly invalid data!");
		}
	}
	
	samples_since_last_pulse++;
	samples_since_last_delay_update++;
//	reflected_last = reflected;	
	if(sampling_delay_ms > sample_still_delay_ms)
	{
		sampling_delay_ms = sample_still_delay_ms;
	}
	return sampling_delay_ms;
}	
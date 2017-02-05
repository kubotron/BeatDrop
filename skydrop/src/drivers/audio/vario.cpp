#include "vario.h"

#include "../uart.h"
#include "../../fc/fc.h"
#include "buzzer.h"
#include "audio.h"
#include "sequencer.h"

#define AUDIO_LOW_PASS		10.0

#define VARIO_OFF	0
#define VARIO_BEEP	1
#define VARIO_PAUSE	2
#define VARIO_CONT	3
#define VARIO_BIBIP 4

volatile uint8_t audio_vario_mode = VARIO_OFF;
volatile uint16_t audio_vario_pause;
volatile uint16_t audio_vario_length;
volatile float audio_vario_freq = 0;

int16_t vario_ivario_old = 0;
bool vario_force_change = false;

uint16_t black_keys[41] = {
26  ,
29  ,
34  ,
39  ,
46  ,
52  ,
58  ,
69  ,
78  ,
92  ,
104 ,
116 ,
139 ,
155 ,
185 ,
207 ,
233 ,
277 ,
311 ,
370 ,
415 ,
466 ,
554 ,
622 ,
740 ,
831 ,
932 ,
1109,
1244,
1479,
1661,
1865,
2217,
2489,
2960,
3322,
3729,
4435,
4978,
4978,
4978};

uint16_t black_keys_shift[41] = {
29  ,
34  ,
39  ,
46  ,
52  ,
58  ,
69  ,
78  ,
92  ,
104 ,
116 ,
139 ,
155 ,
185 ,
207 ,
233 ,
277 ,
311 ,
370 ,
415 ,
466 ,
554 ,
622 ,
740 ,
831 ,
932 ,
1109,
1244,
1479,
1661,
1865,
2217,
2489,
2960,
3322,
3729,
4435,
4978,
4978,
4978,
4978};

uint16_t bibip_pauses[41] = {
250,
300,
350,
400,
450,
500,
550,
600,
650,
700,
750,
800,
850,
900,
950,
1000,
1050,
1100,
1150,
1200,
1200,
1200,
1150,
1100,
1050,
1000,
950,
900,
850,
800,
750,
700,
650,
600,
550,
500,
450,
400,
350,
300,
250
};

int16_t bibip_freq1;
int16_t bibip_freq2;

extern Timer audio_timer;


//linear aproximation between two points
uint16_t get_near(float vario, volatile uint16_t * src)
{
	vario = vario * 2; //1 point for 50cm
	float findex = floor(vario) +  20;
	float m = vario - floor(vario);

	uint8_t index = findex;

	if (findex > 39)
	{
		index = 39;
		m = 1.0;
	}

	if (findex < 0)
	{
		index = 0;
		m = 0.0;
	}

	int16_t start = src[index];

	start = start + (float)((int16_t)src[index + 1] - start) * m;

	return start;
}

ISR(AUDIO_TIMER_OVF)
{
	if (audio_vario_mode == VARIO_BEEP)
	//pause start
	{
		//silent
		buzzer_set_vol(0);

		if (audio_vario_pause > 0)
		{
			audio_timer.SetTop(audio_vario_pause);
			audio_vario_mode = VARIO_PAUSE;
		}
		else
			audio_off();

		return;
	}

	if (audio_vario_mode == VARIO_PAUSE)
	//sound start
	{
		buzzer_set_vol(config.gui.vario_volume);
		buzzer_set_freq(audio_vario_freq);

		if (audio_vario_length > 0)
		{
			audio_timer.SetTop(audio_vario_length);
			audio_vario_mode = VARIO_BEEP;
		}
		else
			audio_off();

		return;
	}
	
	if (audio_vario_mode == VARIO_BIBIP)
	{
	    
	}
}

void audio_vario_apply()
{
	switch (audio_vario_mode)
	{
	    case(VARIO_BIBIP): 
	        //todo
	        seq_bibip(bibip_freq1,bibip_freq2,audio_vario_pause);
	        break;
		case(VARIO_OFF):
			//start the beeps
			if (audio_vario_length > 0 && audio_vario_pause > 0)
			{
				buzzer_set_vol(config.gui.vario_volume);
				buzzer_set_freq(audio_vario_freq);

				audio_timer.SetValue(0);
				audio_timer.SetTop(audio_vario_length);
				audio_timer.Start();

				audio_vario_mode = VARIO_BEEP;
				break;
			}
			//continous tone
			else
			{
				buzzer_set_vol(config.gui.vario_volume);
				buzzer_set_freq(audio_vario_freq);

				audio_vario_mode = VARIO_CONT;
				break;
			}

		break;

		case(VARIO_BEEP):
			if (audio_vario_length == 0 || audio_vario_pause == 0)
			{
				audio_timer.Stop();
				buzzer_set_vol(config.gui.vario_volume);
				buzzer_set_freq(audio_vario_freq);

				audio_vario_mode = VARIO_CONT;
				break;
			}

			if (config.audio_profile.fluid)
				buzzer_set_freq(audio_vario_freq);
		break;

		case(VARIO_PAUSE):
			if (vario_force_change)
				audio_vario_mode = VARIO_OFF;
		break;

		case(VARIO_CONT):
			if (audio_vario_length > 0 && audio_vario_pause > 0)
			{
				buzzer_set_freq(audio_vario_freq);

				audio_timer.SetValue(0);
				audio_timer.SetTop(audio_vario_length);
				audio_timer.Start();

				audio_vario_mode = VARIO_BEEP;
				break;
			}

			buzzer_set_freq(audio_vario_freq);
		break;
	}
}


void audio_vario_step(float vario)
{
	if (config.gui.vario_mute || config.gui.silent & (1 << active_page))
	{
		audio_off();
		return;
	}

	//climb is float in m/s
	int16_t ivario = vario * 100;

	vario_force_change = (abs(ivario - vario_ivario_old) >= 10) ? true: false;
	vario_ivario_old = ivario;
	
	if (audio_vario_mode == VARIO_BIBIP){
	   if (ivario > 0.5)//lift
	   {
            bibip_freq1 = get_near(vario, black_keys_shift);
            bibip_freq2 = get_near(vario, black_keys);	       
	   }
	   else if (ivario > -1.5)//buoyant
	   {
            bibip_freq1 = get_near(vario, black_keys);
            bibip_freq2 = get_near(vario, black_keys);	       
	   }
	   else //sink
	   {
    	    bibip_freq1 = get_near(vario, black_keys);
            bibip_freq2 = get_near(vario, black_keys_shift);
	   }
	   audio_vario_pause = get_near(vario, bibip_pauses);
	   audio_vario_apply();
	   return;
	}


	//buzzer
	if (config.vario.weak_lift_enabled)
	{
	       
		int16_t buzz_thold = config.audio_profile.lift - config.vario.weak_lift;

		if (ivario >= buzz_thold && ivario < config.audio_profile.lift && ivario > config.audio_profile.sink)
		{
			int16_t freq;

			//addition to base weak lift freq (can be negative)
			int16_t beep_freq = get_near(config.audio_profile.lift / 100.0, config.audio_profile.freq);
			beep_freq -= config.audio_profile.weak_lift_freq;

			freq = config.audio_profile.weak_lift_freq + ((int32_t)beep_freq * (int32_t)(ivario - buzz_thold)) / (int32_t)config.vario.weak_lift;

			if (audio_vario_freq != 0)
				audio_vario_freq += ((float)freq - audio_vario_freq) / AUDIO_LOW_PASS;
			else
				audio_vario_freq = freq;

			audio_vario_length = 0;
			audio_vario_pause = 0;
			audio_vario_apply();

			return;
		}
	}

	if ((ivario >= config.audio_profile.lift || ivario <= config.audio_profile.sink) && (config.gui.vario_volume > 0))
	{
		//get frequency from the table
		uint16_t freq = get_near(vario, config.audio_profile.freq);

		if (audio_vario_freq != 0)
			audio_vario_freq += ((float)freq - audio_vario_freq) / AUDIO_LOW_PASS;
		else
			audio_vario_freq = freq;

		//convert ms to timer ticks
		audio_vario_length = get_near(vario, config.audio_profile.length) * 31;
		audio_vario_pause = get_near(vario, config.audio_profile.pause) * 31;

		//update audio with new settings
		audio_vario_apply();

		return;
	}	

	//no threshold was exceeded -> silent
	audio_off();
}

void audio_vario_reset()
{
	//so lowpass will not affect affect new beeps
	audio_vario_freq = 0;
	//next vario sound will go from OFF state
	audio_vario_mode = VARIO_BIBIP;

}

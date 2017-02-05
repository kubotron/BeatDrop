/*
 * sequencer.cpp
 *
 *  Created on: 3.9.2015
 *      Author: horinek
 */

#include "sequencer.h"

#include "../uart.h"
#include "../../fc/fc.h"
#include "buzzer.h"
#include "audio.h"

//sequencer
volatile bool seq_enabled = false;
volatile bool bibip_mode = false;

volatile const uint16_t * seq_tone_ptr;
volatile const uint16_t * seq_length_ptr;
volatile uint8_t seq_index;
volatile uint8_t seq_len;
volatile uint16_t seq_duration;
volatile uint8_t seq_volume;

volatile uint16_t bip_freq1;
volatile uint16_t bip_freq2;
volatile uint16_t bibip_pause;

#define BIBIP_GAP  500
#define ENVELOPE_LEN 42
#define ENVELOPE_INDEX_GAP 20
#define ENVELOPE_INDEX_PAUSE 41

const uint16_t env_durations[ENVELOPE_LEN] = {
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
BIBIP_GAP,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
10,
0};

const uint16_t env_volumes[ENVELOPE_LEN] = {
30,
60,
90,
80,
70,
50,
45,
40,
35,
30,
10,
30,
10,
30,
10,
30,
10,
30,
10,
30,
0,
30,
60,
90,
80,
70,
50,
45,
40,
35,
30,
10,
30,
10,
30,
10,
30,
10,
30,
10,
30,
0};


#define AUDIO_SILENT_AFTER_SEQ	250

void seq_bibip(uint16_t freq1, uint16_t freq2, uint16_t pause)
{
    audio_off();
    seq_enabled = true;
    bibip_mode = true;

    seq_len = ENVELOPE_LEN;
    bip_freq1 = freq1;
    bip_freq2 = freq2;
    bibip_pause = pause;

    seq_index = 0;   
}

void seq_next_env()
{
    uint16_t tone;
    if (seq_index < seq_len)
    {
        if (seq_index < ENVELOPE_INDEX_GAP)
        {
            tone = bip_freq1;
        }
        else if (seq_index == ENVELOPE_INDEX_GAP)
        {
            tone = 0;
        }
        else if (seq_index > ENVELOPE_INDEX_GAP && seq_index < ENVELOPE_INDEX_PAUSE)
        {
            tone = bip_freq2;
        } 
        else if (seq_index == ENVELOPE_INDEX_PAUSE){
            tone = 0;
        }
        
        if (seq_index == ENVELOPE_INDEX_PAUSE){
            seq_duration = bibip_pause;
            seq_volume = 0; 
        } else {
            seq_duration = env_durations[seq_index];
            seq_volume = env_volumes[seq_index];
        }
    }
    else
    {
        //this will separate sequence end from vario sound
        tone = 0;
        seq_duration = AUDIO_SILENT_AFTER_SEQ;
    }

    seq_index++;
    buzzer_set_vol(seq_volume);
    buzzer_set_freq(tone);
}

void seq_start(const sequence_t * seq, uint8_t volume)
{
	audio_off();
	seq_enabled = true;
	bibip_mode = false;

	seq_len = pgm_read_byte(&seq->length);
	seq_tone_ptr = (const uint16_t*)pgm_read_word(&seq->tone_ptr);
	seq_length_ptr = (const uint16_t*)pgm_read_word(&seq->length_ptr);
	seq_index = 0;
	seq_volume = volume;
}

void seq_next_tone()
{
	uint16_t tone;
	if (seq_index < seq_len)
	{
		//load tone and length from pgm
		tone = pgm_read_word(&seq_tone_ptr[seq_index]);
		seq_duration = pgm_read_word(&seq_length_ptr[seq_index]);
	}
	else
	{
		//this will separate sequence end from vario sound
		tone = 0;
		seq_duration = AUDIO_SILENT_AFTER_SEQ;
	}

	seq_index++;

	buzzer_set_vol(seq_volume);
	buzzer_set_freq(tone);
}

//audio_step @ 100Hz (called from fc meas_timer)
#define AUDIO_STEP_MS	10

void seq_loop()
{
	if (seq_duration > AUDIO_STEP_MS)
	{
		seq_duration -= AUDIO_STEP_MS;
	}
	else
	{
		if (seq_index == seq_len + 1)
		{
			seq_enabled = false;
			audio_off();
		}
		else if (bibip_mode)
		{
		    seq_next_env();
		}
		else
		{
			seq_next_tone();
		}
	}
};

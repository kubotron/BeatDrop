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
volatile bool beebeep_mode = false;

volatile const uint16_t * seq_tone_ptr;
volatile const uint16_t * seq_length_ptr;
volatile uint8_t seq_index;
volatile uint8_t seq_len;
volatile uint16_t seq_duration;
volatile uint8_t seq_volume;

volatile uint16_t bip_freq1;
volatile uint16_t bip_freq2;
volatile uint16_t bibip_pause;

#define BIBIP_GAP  40
#define ENVELOPE_LEN 102
#define ENVELOPE_INDEX_GAP 50
#define ENVELOPE_INDEX_PAUSE 101
#define BIBIP_SAMPLE_LEN 1

const uint16_t env_durations[ENVELOPE_LEN] = {
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_GAP,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
BIBIP_SAMPLE_LEN,
0};

const uint16_t env_volumes[ENVELOPE_LEN] = {
16,
24,
32,
44,
48,
56,
64,
76,
81,
78,
73,
70,
65,
61,
57,
52,
49,
45,
42,
42,
41,
41,
41,
41,
41,
40,
40,
40,
40,
39,
39,
36,
34,
33,
31,
30,
29,
27,
25,
23,
21,
19,
16,
14,
11,
9,
6,
5,
3,
2,
0,
16,
24,
32,
44,
48,
56,
64,
76,
81,
78,
73,
70,
65,
61,
57,
52,
49,
45,
42,
42,
41,
41,
41,
41,
41,
40,
40,
40,
40,
39,
39,
36,
34,
33,
31,
30,
29,
27,
25,
23,
21,
19,
16,
14,
11,
9,
6,
5,
3,
2,
0};

#define AUDIO_SILENT_AFTER_SEQ	250

#define BEEP_LEN 42
#define BEEP 20
#define BEEP_GAP 5 
#define BEEP_INDEX_GAP 20
#define BEEP_INDEX_PAUSE 41 

void seq_beeb_beep(uint16_t freq1, uint16_t freq2, uint16_t pause)
{
    audio_off();
    beebeep_mode = true;
    seq_enabled = true;
    bibip_mode = false;

    seq_len = BEEP_LEN;
    bip_freq1 = freq1;
    bip_freq2 = freq2;
    bibip_pause = pause;
    
    seq_index = 0;
    seq_volume = 50;
}

void seq_bibip(uint16_t freq1, uint16_t freq2, uint16_t pause)
{
    audio_off();
    seq_enabled = true;
    bibip_mode = true;
    beebeep_mode = false;

    seq_len = ENVELOPE_LEN;
    bip_freq1 = freq1;
    bip_freq2 = freq2;
    bibip_pause = pause;

    seq_index = 0;   
}


void seq_next_beep()
{
    uint16_t tone;
    if (seq_index < seq_len)
    {
    
        if (seq_index < BEEP_INDEX_GAP)
        {           
            seq_duration = BEEP;
            tone = bip_freq1;            
        }
        else if (seq_index == BEEP_INDEX_GAP)
        {
            seq_duration = BEEP_GAP;
            tone = 0;
        }
        else if (seq_index > BEEP_INDEX_GAP && seq_index < BEEP_INDEX_PAUSE)
        {
            seq_duration = BEEP;
            tone = bip_freq2;            
        } 
        else if (seq_index == BEEP_INDEX_PAUSE){            
            tone = 0;           
        }
        
        if (seq_index == BEEP_INDEX_PAUSE){
            seq_duration = bibip_pause;
            seq_volume = 0; 
        } else {
            seq_volume = 40;
        }               
    }
    else
    {
        //this will separate sequence end from vario sound
        tone = 0;
        seq_duration = 1;
    }

    seq_index++;

    buzzer_set_vol(seq_volume);
    buzzer_set_freq(tone);
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
            tone = 0;
            seq_duration = 1;
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
	beebeep_mode = false;

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
    if (tone < 2800){
	   buzzer_set_vol(seq_volume);
	} else if (tone < 3100) {
	   buzzer_set_vol(seq_volume/2);
	} else {
	   buzzer_set_vol(seq_volume/4);
	}
	buzzer_set_freq(tone);
}


//audio_step @ 100Hz (called from fc meas_timer)    
#define AUDIO_STEP_MS	15

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
		else if (beebeep_mode)
        {
            seq_next_beep();
        }
		else
		{
			seq_next_tone();
		}
	}
};

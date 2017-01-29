
#ifndef BIBIP_H_
#define BIBIP_H_

#define BIBIP_ENVELOPE_MODE

#define BIBIP_LENGTH_MS 200 
#define BIBIP_PAUSE_MS  50 

//approximately pentatonical scale
#define BIBIP_FREQ_0  220 
#define BIBIP_FREQ_1  248
#define BIBIP_FREQ_2  278
#define BIBIP_FREQ_3  330
#define BIBIP_FREQ_4  371
#define BIBIP_FREQ_5  440
#define BIBIP_FREQ_6  495
#define BIBIP_FREQ_7  557
#define BIBIP_FREQ_8  660
#define BIBIP_FREQ_9  743
#define BIBIP_FREQ_10 880
#define BIBIP_FREQ_11 990
#define BIBIP_FREQ_12 1114

#define BIBIP_ATTACK 40
#define BIBIP_SUSTAIN 10

#define BIBIP_SEQ ARR({BIBIP_LENGTH_MS, BIBIP_PAUSE_MS, BIBIP_LENGTH_MS})

#define BIBIP_ENV ARR({BIBIP_ATTACK, 0, BIBIP_SUSTAIN})

MK_SEQ(vario_seq1, ARR({BIBIP_FREQ_0, 0, BIBIP_FREQ_1}), BIBIP_SEQ);
MK_SEQ(vario_seq2, ARR({BIBIP_FREQ_10, 0, BIBIP_FREQ_9}), BIBIP_SEQ);
MK_SEQ_ENV(env_seq, ARR({BIBIP_FREQ_10, 0, BIBIP_FREQ_9}), BIBIP_SEQ, BIBIP_ENV);


extern const sequence_t * active_seq;

extern volatile uint16_t tone1;
extern volatile uint16_t tone2;

#endif /* BIBIP_H_ */

/*
 * vario.h
 *
 *  Created on: 3.9.2015
 *      Author: horinek
 */

#ifndef AUDIO_VARIO_H_
#define AUDIO_VARIO_H_

#include "../../common.h"

void audio_vario_step(float vario);
void audio_vario_reset();

extern volatile float audio_vario_freq;
extern volatile uint16_t audio_vario_pause;
extern volatile uint16_t audio_vario_length;
extern volatile int16_t bibip_freq1;
extern volatile int16_t bibip_freq2;

#endif /* VARIO_H_ */

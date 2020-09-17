/*
 * tone.c
 *
 *  Created on: Feb 12, 2014
 *      Author: HollyWynn
 */
#include "tone.h"
#include "chip.h"
#include "delay.h"

void tone_Init(void)
{
	/* Enable timer 1 clock */
		Chip_TIMER_Init(LPC_TIMER32_1);

		/* Timer rate is system clock rate */
		//timerFreq = Chip_Clock_GetSystemClockRate();

		/* Reset counts and match values to zero */
		Chip_TIMER_Reset(LPC_TIMER32_1);
		//Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1); // We do not want an interrupt
		//Chip_TIMER_SetMatch(LPC_TIMER32_1, 1, (timerFreq / basefreq)); // 1200 Hz hopefully
		Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_1, 1); // Enable counter reset
		Chip_TIMER_ExtMatchControlSet(LPC_TIMER32_1, 0,TIMER_EXTMATCH_TOGGLE, 1); //setup match register
		//Chip_TIMER_Enable(LPC_TIMER32_1);

		/* Enable timer interrupt */
		//NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
		//NVIC_EnableIRQ(TIMER_32_0_IRQn);
		return;
}

void play_tone(int freq, int duration)
{
	Chip_TIMER_Reset(LPC_TIMER32_1);
	Chip_TIMER_SetMatch(LPC_TIMER32_1, 1, (48000000 / (freq *2)));
	Chip_TIMER_Enable(LPC_TIMER32_1);
	delay(duration);
	Chip_TIMER_Disable(LPC_TIMER32_1);
	return;
}

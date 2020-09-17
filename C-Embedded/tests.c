/*
 * tests.c
 *
 *  Created on: Mar 14, 2014
 *      Author: HollyWynn
 */

#include "tests.h"
#include "pins.h"
#include "delay.h"
#include "tone.h"
#include "relays.h"
#include "board.h"
#include "chip.h"

void self_test(void)
{
	//Toggles LEDs and Relays
	r_write_outputs("I000");
	delay(500);
	r_write_outputs("II00");
	delay(500);
	r_write_outputs("III0");
	delay(500);
	r_write_outputs("IIII");
	delay(500);
	set_out(LED1);
	delay(500);
	set_out(LED2);
	delay(500);
	set_out(SILENT);
	delay(500);
	play_tone(4000, 1500);
	delay(500);
	r_write_outputs("0000");
	delay(500);
	clr_out(LED1);
	delay(500);
	clr_out(LED2);
	return;
}

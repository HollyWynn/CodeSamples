/*
 * setup_funcs.c
 *
 *  Created on: Mar 14, 2014
 *      Author: HollyWynn
 */

#include "setup_funcs.h"
#include <cr_section_macros.h>
#include "tests.h"
#include "CAN.h"
#include "serial.h"
#include "bluetooth.h"
#include "delay.h"
#include "board.h"
#include "chip.h"
#include "mgc.h"
#include "tone.h"
#include "inputs.h"
#include "extras.h"


#define ID_NUM_BITS 2
char ID[ID_NUM_BITS];
int is_master = 0;
int address = 0;
int op_mode = 0;


int sf_config_op_mode(void)
{// Reads dip switch positions on start up and returns operating mode
	// Dip1 0 = CAN Bridge 1 = Wireless cable (passes input status of master to output relays of slave)
	// Dip2 if Dip1 = 1, 0 = SLave, 1 = Master
	// Dip3 and 4 if Dip 1 = 1, Address bits.
	op_mode = i_read_switch(1);
	if (op_mode == 1)
	{
		is_master = i_read_switch(2);
		ID[0] = IorZero(i_read_switch(3));
		ID[1] = IorZero(i_read_switch(4));
	}
	return op_mode;
}

char* sf_get_ID(void)
{
	return ID;
}

int sf_is_master(void)
{
	return is_master;
}

void sf_setup(void)
{
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();

	/* Enable and setup SysTick Timer for mS ticks */
	SysTick_Config(SystemCoreClock / 1000);

	// Set up board specific options and peripherals
	Board_Init();
	Serial_Init();
	tone_Init();
	self_test();
}

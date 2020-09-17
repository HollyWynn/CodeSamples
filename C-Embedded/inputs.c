/*
 * inputs.c
 *
 *  Created on: Mar 14, 2014
 *      Author: HollyWynn
 */

#include "inputs.h"
#include "pins.h"
#include "board.h"
#include "chip.h"
#include "extras.h"

int input_lookup_x[] = {3,2,2,3, 1,1,2, 1,0,2, 2};
int input_lookup_y[] = {3,6,0,2,11,4,3,10,8,2,10};

char out_str[4];


// All inputs/buttons/ switches are ACTIVE LOW, make sure all logic is reversed to return pressed/on = true
// inputs
char* i_read_inputs(void)
{
	int i;
	for(i=3;i<7;i++)
	{
		out_str[i-3] = IorZero(!(read_pin(input_lookup_x[i], input_lookup_y[i])));
	}
	return out_str;
}

int i_read_input(int input_num)
{
	return !(read_pin(input_lookup_x[input_num+2], input_lookup_y[input_num+2]));
}

// buttons
char* i_read_buttons(void)
{
	int i;
	for(i=0;i<3;i++)
	{
		out_str[i] = IorZero(!read_pin(input_lookup_x[i], input_lookup_y[i]));
	}
	return out_str;
}

int i_read_button(int input_num)
{
	return !(read_pin(input_lookup_x[input_num-1], input_lookup_y[input_num-1]));
}

// switches
char* i_read_switches(void)
{
	int i;
	for(i=7;i<11;i++)
	{
		out_str[i-7] = IorZero(!(read_pin(input_lookup_x[i], input_lookup_y[i])));
	}
	return out_str;
}

int i_read_switch(int input_num)
{
	return !(read_pin(input_lookup_x[input_num+6], input_lookup_y[input_num+6]));
}

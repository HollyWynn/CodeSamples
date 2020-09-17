/*
 * relays.c
 * Module for control of output relays
 *  Created on: Mar 14, 2014
 *      Author: HollyWynn
 */

#include "relays.h"
#include "board.h"
#include "chip.h"


int relay_lookup_x[] = {1,1,0,2};
int relay_lookup_y[] = {1,0,11,11};

int r_write_outputs(char* bitfield)
{
	// bitfield should be 4 chars long and contain only "I" or "0"(zero)
	int i;
	for(i = 0; i < 4; i++)
	{
		if (bitfield[i] == 'I')
		{
			set_relay(relay_lookup_x[i],relay_lookup_y[i]);
		}
		else
		{
			clr_relay(relay_lookup_x[i],relay_lookup_y[i]);
		}
	}

	return 1;
}


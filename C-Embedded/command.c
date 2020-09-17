/*
 * command_parser.c
 *
 *  Created on: Mar 14, 2014
 *      Author: HollyWynn
 */

#include "tone.h"
#include "can.h"
#include "bluetooth.h"
#include "relays.h"

int locate(void)
{
	play_tone(4000, 1000);
	play_tone(3000, 1000);
	play_tone(4000, 1000);
	return 1;
}

int c_parse_message(char* message)
{
	switch(message[1])
	{
		// C CAN
		case 'C':
			return CAN_set_opt(message);
		break;
		// L for locate (plays tone)
		case 'L':
			return locate();
		break;
		// W for bluetooth Wireless options
		case 'W':
			return bt_set_opt(message);
		break;
		// R for relays
		case 'R':
			return r_write_outputs(message+2);
		break;
		// Below options are a special case because they return a non-true/false value
		// I for inputs
		/*
		case 'I':
			// Read inputs, Display I or 0 on input high or low
			// IN1 IN2 IN3 IN4
			arraycpy(default_out_str, out_str,21);
			for(i=3;i<7;i++)
			{
				out_str[i-3] = IorZero(read_pin(input_lookup_x[i], input_lookup_y[i]));
			}
		break;
		// S for switches
		case 'S':
			// Sw1 SW2 Sw3 Sw4
			arraycpy(default_out_str, out_str,21);
			for(i=7;i<11;i++)
			{
				out_str[i-7] = IorZero(read_pin(input_lookup_x[i], input_lookup_y[i]));
			}

		break;
		// B for buttons
		case 'B':
			// Red button Blue button Black button
			arraycpy(default_out_str, out_str,21);
			for(i=0;i<3;i++)
			{
				out_str[i] = IorZero(read_pin(input_lookup_x[i], input_lookup_y[i]));
			}

		break;
		*/
		// Unkown Command
		default:
			return 0;
		break;
	}
	return 0;
}

/*
 * mgc.c
 *
 *  Created on: Feb 13, 2014
 *      Author: HollyWynn
 */

#include "can.h"
#include "extras.h"
#include "delay.h"

#define MAX_WAIT_TIME 2000 // Time to wait for response before returning.

void get_serial_num(char* dest_array)
{
	/*
	 * 601 8 40 18 10 04 00 00 00 00 //Send
       581 8 41 18 10 04 0a 00 00 00 //Recv		0x0a bytes
       601 8 60 00 00 00 00 00 00 00 //Send
       581 8 00 45 31 30 30 31 32 31 //Recv		00 E100121
       601 8 70 00 00 00 00 00 00 00 //Send
       581 8 19 33 34 00 00 00 00 00 //Recv		19 34
	 *
	 */
	long time_started = 0;
	char tmp[10];
	char serial_num[10];
	char default_ser_num[] = "BTDModule\r";
	bool success = false;
	// Set filter to 0x581 ONLY
	msg581only = true;
	// Send first command
	response_msg = false;
	time_started = millis();
	CAN_Send("06014018100400000000");
	// Check response
	while(((!(response_msg)) && ((millis() - time_started) <= MAX_WAIT_TIME))){}
	CAN_Get_msg(tmp); // This response just tells us that we will get 9 bytes
	// Send second command
	response_msg = false;
	time_started = millis();
	CAN_Send("06016000000000000000");
	// check response
	while(((!(response_msg)) && ((millis() - time_started) <= MAX_WAIT_TIME))){}
	CAN_Get_msg(tmp); // bytes 3 to 7 have serial number data
	serial_num[0] = tmp[3];
	serial_num[1] = tmp[4];
	serial_num[2] = tmp[5];
	serial_num[3] = tmp[6];
	serial_num[4] = tmp[7];
	serial_num[5] = tmp[8];
	serial_num[6] = tmp[9];
	// send last command
	response_msg = false;
	time_started = millis();
	CAN_Send("06017000000000000000");
	// check response
	while(((!(response_msg)) && ((millis() - time_started) <= MAX_WAIT_TIME))){}
	CAN_Get_msg(tmp); // bytes 3 and 4 have serial number data
	serial_num[7] = tmp[3];
	serial_num[8] = tmp[4];
	serial_num[9] = '\r';
	// serial_num is already in ASCII

	// place in dest_array
	if(serial_num[0] == 'E')
		{
		arraycpy(serial_num, dest_array, 10);
		}
	else
	{
		arraycpy(default_ser_num, dest_array, 10);
	}
	msg581only = false;
	return;
}

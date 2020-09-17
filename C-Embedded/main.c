/*
===============================================================================
 Name        : main.c
 Author      : Holly Wynn
 Version     : A
===============================================================================
*/

#include "board.h"
#include "chip.h"
#include "pins.h"
#include "itoa.h"
#include "atoi.h"
#include "serial.h"
#include "can.h"
#include "delay.h"
#include "extras.h"
#include "tests.h"
#include "setup_funcs.h"
#include "command.h"
#include "inputs.h"
#include "bluetooth.h"
#include "mgc.h"
#include "relays.h"

#define TIMEOUT_VALUE 10000 // Serial inter-character timeout in milliseconds
#define MSG_LEN 21

// The on chip ROM drivers need some RAM to work, must be reserved or else!
__BSS(RESERVED) char CAN_driver_memory[256] ;

bool filter_msgs = true;


int bytesread = 0;
int x = 0;
char c = 0;
bool msg_OK = false;
bool enable_timeout = false;
char tmp[2];
char msg_buff[21];
char default_out_str[] = "                    \r";
char out_str[21];

long last_char_time = 0;
long timeout_timer = 0;

// String Table, All messages are 21 bytes

char OK[] =      "OK        0000000000\r";
char BAD_MSG[] = "E NAK BAD MESSAGE 00\r";
char BAD_OPT[] = "E NAK BAD OPTION 000\r";
char TIME_OUT[] ="\rE TIMEOUT 000000000\r";

void msg_reset(void)
{
	c = 0;
	bytesread = 0;
	msg_OK = false;
}

void get_message(void)
{
	msg_OK = false;
	c = Serial_ReadChar();
	timeout_timer = (millis() - last_char_time);
	switch (c)
	{
		case '\x00': // No data read

		break;
		case '\x0D':
			enable_timeout = true;
			timeout_timer = 0; // reset timer
			last_char_time = millis();
			if (20 == bytesread)
			{
				enable_timeout = false;
				msg_OK = true;
				break;
			}
			else if (bytesread == 0)
			{// its OK, we're in sync
				break;
			}
			else
			{// We don't have a complete 20 byte message try to regain sync
				flush_rx_ringbuffer();
				msg_reset();
			}
		break;

		default:
			enable_timeout = true;
			timeout_timer = 0; // reset timer
			last_char_time = millis();
			msg_buff[bytesread] = c;
			bytesread++;
			if (bytesread > 21)
			{
				flush_rx_ringbuffer();
				msg_reset();
			}
		break;
	}

	if(enable_timeout &&(timeout_timer > TIMEOUT_VALUE))
	{
		flush_rx_ringbuffer();
		msg_reset();
		enable_timeout = false;
	}
	return;
}

int CAN_bridge_main(void)
{
	// CAN bridge specific setup
	char serial_num[10];
	CAN_Init(500000);
	CAN_SetupRx();
	// allmsgs and msg581only are found in CAN.H and used for message filtering
	// Values below set the default filter values on startup.
	allmsgs = false;
	msg581only = false;

	get_serial_num(serial_num);
	bt_change_id(serial_num, 10);
	bluetooth_Init();
	CAN_Enable();
	process_can_msgs = true;
	//main
	last_char_time = millis();
	last_sent_timer = 0;
	while(1)
	{

		if(msg_OK)
		{
			CAN_Send(msg_buff);
			last_sent_timer = 0; // Reset the timer for message 281
			msg_reset();
		}
		// Process received messages from CAN, if any available
		if (CAN_NewMsgCnt)
		{
			if (CAN_NewMsgCnt > 9)
			{
				// We lost messages, not good.
				//set_out(LED1);
			}
			CAN_NewMsgCnt --;
			clear(msg_buff, 21);
			bytestohex(Msg_Stack[CAN_Read_Idx], msg_buff, 10);
			CAN_Read_Idx++;

			if (CAN_Read_Idx >9)
			{
				CAN_Read_Idx = 0;
			}

			msg_buff[20] = '\r';
			Serial_Print(msg_buff, 21);
		}
	}
}

int wireless_cable_main(void)
{
	//Wireless cable specific setup
	bt_reset();
	delay(4000);
	r_write_outputs("000I");
	char ID_string[] = "WirelessC00";
	char* ID_bits = sf_get_ID();
	ID_string[9] = ID_bits[0];
	ID_string[10] = ID_bits[1];
	bt_change_id(ID_string, 11);
	r_write_outputs("00II");
	delay(5000);
	flush_rx_ringbuffer();
	if (sf_is_master())
	{
		char* slave_address = bt_find_slave(ID_string);
		if(slave_address)
		{
			while(!(bt_bond(slave_address)));
			r_write_outputs("I0II");
			delay(10000);
			while(!(bt_connect(slave_address)));
			r_write_outputs("IIII");
			delay(500);
		}
		else
		{
			set_out(LED1);
			while(1);
		}
	}
	else
	{
		bt_wait_for_connection(); // Slave will burn time until a master connects
	}

	//main
	last_char_time = millis();
    last_sent_timer = 0;
    char old_inputs[4];
    char new_inputs[4];
	r_write_outputs("0000");
    while(1)
	{
		get_message();
		if(msg_OK)
		{
			c_parse_message(msg_buff);
			msg_reset();
		}
		arraycpy(i_read_inputs(), new_inputs, 4);
		if (!(arraycmp(new_inputs, old_inputs, 4)))
		{
			// send command to paired module
			Serial_Print("CR", 2);
			Serial_Print(new_inputs, 4);
			Serial_Print("00000000000000\r", 15);
			arraycpy(new_inputs, old_inputs, 4);
		}
	}
}

int module_spoof_main(void)
{// Main program for Module research
	// Provides CAN filtering based on Module address
	// also handles heartbeat automatically
	// Intended to be used with PC-side software for module emulation
	return 0;
}

int main(void) {
	sf_setup();
	flush_rx_ringbuffer();// make sure we clear any garbage from the buffer
	int run_mode = sf_config_op_mode();
	switch(run_mode)
	{
	case 0:
		CAN_bridge_main();
	break;
	case 1:
		wireless_cable_main();
	break;
	}

	return 0 ;
}

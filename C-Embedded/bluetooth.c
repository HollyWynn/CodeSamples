/*
 * bluetooth.c
 *
 *  Created on: Feb 13, 2014
 *      Author: HollyWynn
 */

#include "bluetooth.h"
#include "serial.h"
#include "extras.h"
#include "delay.h"
#include "atoi.h"

#define BUFF_SZ 255
#define MIN_MATCH_SZ 12

char buff[BUFF_SZ];
char bt_address[12];
int response_len = 0;

// Inquiry/Discovery response strings
char* InqResponse = "AT-AB InqPending";
char* DiscResponse = "AT-AB DiscoveryPending";

// Connect response strings
char* ConResponseOK1 = "AT-AB ConnectionUp";
char* ConResponseOK2 = "AT-AB -BypassMode-";
char* ConResponseERR = "AT-AB SPPConnectionClosed";

// Bond response strings
char* BondResponseOK = "AT-AB BondOK";
char* BondResponseERR = "AT-AB BondFail";

// Requires UART to be initialized and enabled first!

void bluetooth_Init(void) // Send initialization string to BT33
{
	Serial_Print("AT+AB HostEvent D\r" , 18);
}

void bt_wait_for_connection(void)
{
	int connected = 0;
	while(!(connected))
	{
		response_len = Serial_ReadLine(buff, BUFF_SZ);
		if(response_len != 0)
		{
			if (arraycmp(ConResponseOK2, buff, MIN_MATCH_SZ))
			{
				connected = 1;
			}
		}
		delay(500);
		// blink an LED?
	}
}



void bt_change_id(char* new_id, int len)
{
	Serial_Print("AT+AB LocalName " , 16);
	Serial_Print(new_id , len);
	Serial_Print("\r" , 1);
}

// Scan function to find devices, returns the bluetooth address of the first "slave gate" device with matching ID
char* bt_find_slave(char* ID)
{
	Serial_Print("AT+AB Discovery\r", 16);
	// should receive "AT-AB InqPending\r"
	response_len = Serial_ReadLine(buff, BUFF_SZ);
	if(response_len == 0)
	{
		return 0;
	}
	if (!(arraycmp(InqResponse, buff, MIN_MATCH_SZ)))
	{
		return 0;
	}

	// then receive "AT-AB DiscoveryPending X\r" where X is a number from 0 to 10
	response_len = Serial_ReadLine(buff, BUFF_SZ);
	if(response_len == 0)
	{
		return 0;
	}
	if (!(arraycmp(DiscResponse, buff, MIN_MATCH_SZ)))
	{
		return 0;
	}
	int x = hatoi(buff[23]);
	if(x) // then we get X number of address name pairs
	{
		for(int i=0; i < x; i++)
		{
			response_len = Serial_ReadLine(buff, BUFF_SZ);

			// AT-AB Device Addr "Name"\r where address is the bluetooth device address in hex
			//and name is a string in quotes
			if(response_len)
			{
				// check it for proper ID
				//AT-AB Device 0080e1ff758c "WirelessCXX"
				if(arraycmp(ID, &buff[27],11))
				{
					// return the BT address
					//bt_addr starts at buffer[13] an is (should be) 12 characters when shown as ascii hex
					arraycpy(&buff[13], bt_address, 12);
					return bt_address;
				}
			}
		}
	}
	// if we get here either we did not find our slave device or there was an error
	return 0;
}

int bt_connect(char* bt_addr)
{
	// initiates a connection to the bluetooth device at bt_addr
	Serial_Print("AT+AB SPPConnect ", 17);
	Serial_Print(bt_addr, 12);
	Serial_Print("\r", 1);
	response_len = Serial_ReadLine(buff, BUFF_SZ);
	if(response_len == 0)
	{
		return 0;
	}
	return !(arraycmp(ConResponseERR, buff, MIN_MATCH_SZ));
}

int bt_bond(char* bt_addr)
{
	//initiates bonding with the device specified by bt_addr
	//"AT+AB Bond bt_addr pin\r"
	// expect AT-AB BondOK or AT-AB BondFail
	Serial_Print("AT+AB Bond ", 11);
	Serial_Print(bt_addr, 12);
	Serial_Print(" 1234\r", 6);
	response_len = Serial_ReadLine(buff, BUFF_SZ);// Catch "BondPending" message
	response_len = Serial_ReadLine(buff, BUFF_SZ);// should be BondOK message
	if(response_len == 0)
	{
		return 0;
	}
	return !(arraycmp(BondResponseERR, buff, MIN_MATCH_SZ));
}

void bt_reset(void)
{
	Serial_Print("AT+AB Reset\r", 12);
	response_len = Serial_ReadLine(buff, BUFF_SZ); // Reset Pending
	response_len = Serial_ReadLine(buff, BUFF_SZ); // -CmmandMode-
	response_len = Serial_ReadLine(buff, BUFF_SZ); // BDAddress
	return;
}

int bt_set_opt(char* dummy)
{
	return 0;
}

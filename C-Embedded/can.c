/*
 * can.c
 *
 *  Created on: Oct 23, 2013
 *      Author: HollyWynn
 */


#include "atoi.h"
#include "can.h"
#include "board.h"
#include "chip.h"
#include "extras.h"
#include "delay.h"

#define MAX_INTERVAL 10000

int CAN_Write_Idx = 0;
int CAN_Read_Idx = 0;
int CAN_NewMsgCnt = 0;
char this_msg[10];
char last_msg[10];
long last_millis;



void baudrateCalculate(uint32_t baud_rate, uint32_t *can_api_timing_cfg)
{
	uint32_t pClk, div, quanta, segs, seg1, seg2, clk_per_bit, can_sjw;
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CAN);
	pClk = Chip_Clock_GetMainClockRate();

	clk_per_bit = pClk / baud_rate;

	for (div = 0; div <= 15; div++)
	{
		for (quanta = 1; quanta <= 32; quanta++)
		{
			for (segs = 3; segs <= 17; segs++)
			{
				if (clk_per_bit == (segs * quanta * (div + 1)))
				{
					segs -= 3;
					seg1 = segs / 2;
					seg2 = segs - seg1;
					can_sjw = seg1 > 3 ? 3 : seg1;
					can_api_timing_cfg[0] = div;
					can_api_timing_cfg[1] =
						((quanta - 1) & 0x3F) | (can_sjw & 0x03) << 6 | (seg1 & 0x0F) << 8 | (seg2 & 0x07) << 12;
					return;
				}
			}
		}
	}
}

// CAN Message stacking, places messages in a stack for processing outside of the ISR
void stack(void)
{
	Msg_Stack[CAN_Write_Idx][0] = msg_obj.mode_id >> 8;
	Msg_Stack[CAN_Write_Idx][1] = msg_obj.mode_id;
	Msg_Stack[CAN_Write_Idx][2] = msg_obj.data[0];
	Msg_Stack[CAN_Write_Idx][3] = msg_obj.data[1];
	Msg_Stack[CAN_Write_Idx][4] = msg_obj.data[2];
	Msg_Stack[CAN_Write_Idx][5] = msg_obj.data[3];
	Msg_Stack[CAN_Write_Idx][6] = msg_obj.data[4];
	Msg_Stack[CAN_Write_Idx][7] = msg_obj.data[5];
	Msg_Stack[CAN_Write_Idx][8] = msg_obj.data[6];
	Msg_Stack[CAN_Write_Idx][9] = msg_obj.data[7];
	CAN_Write_Idx++;
	if (CAN_Write_Idx > 9)
	{
		CAN_Write_Idx = 0;
	}
	CAN_NewMsgCnt++;
	Board_LED_Toggle(0);
}

bool compare_msgs(char* msg1, char* msg2)
{
	int i;

	for (i=0;i<10;i++)
	{
		if (msg1[i] != msg2[i])
		{
			return false;
		}
	}
	return true;
}
/*	CAN receive callback */
/*	Function is executed by the Callback handler after
    a CAN message has been received */
void CAN_rx(uint8_t msg_obj_num) {
	// Disable the CAN interrupt to prevent re-entrance issues
	CAN_Disable();
	/* Determine which CAN message has been received */
	msg_obj.msgobj = msg_obj_num;
	/* Now load up the msg_obj structure with the CAN message */
	LPC_CCAN_API->can_receive(&msg_obj);
	switch(msg_obj_num)
	{
		case 1: // Msg 581
			stack();
			if(msg581only)
			{
				response_msg = true;
			}
			break;
		/*case 2: // Msg 281
			if((!(msg581only)) || allmsgs)
			{
				this_msg[0] = msg_obj.mode_id >> 8;
				this_msg[1] = msg_obj.mode_id;
				this_msg[2] = msg_obj.data[0];
				this_msg[3] = msg_obj.data[1];
				this_msg[4] = msg_obj.data[2];
				this_msg[5] = msg_obj.data[3];
				this_msg[6] = msg_obj.data[4];
				this_msg[7] = msg_obj.data[5];
				this_msg[8] = msg_obj.data[6];
				this_msg[9] = msg_obj.data[7];
				// Compare to the last one and check the timer
				last_sent_timer += (millis() - last_millis);
				if ((last_sent_timer > MAX_INTERVAL) || (false == compare_msgs(this_msg, last_msg)))
				{
					stack();
					arraycpy(this_msg, last_msg, 10);
					last_sent_timer = 0;
					last_millis = millis();
				}
			}
			break;*/

		case 3: // Msg 580 - 5FF
			if((!(msg581only)) || allmsgs)
			{
				stack();
			}
			break;

		case 4: // Msg 680 - 67F
			if((!(msg581only)) || allmsgs)
			{
				stack();
			}
			break;

		case 31: // All other messages
			if((!(msg581only)) && allmsgs)
			{
				stack();
			}
			break;

		default:
			break;
	}

	CAN_Enable();
}

/*	CAN transmit callback */
/*	Function is executed by the Callback handler after
    a CAN message has been transmitted */
void CAN_tx(uint8_t msg_obj_num)
{
	// Blink something maybe?
}

/*	CAN error callback */
/*	Function is executed by the Callback handler after
    an error has occured on the CAN bus */
void CAN_error(uint32_t error_info)
{
	// TODO
}

/**
 * @brief	CCAN Interrupt Handler
 * @return	Nothing
 * @note	The CCAN interrupt handler must be provided by the user application.
 *	It's function is to call the isr() API located in the ROM
 */
void CAN_IRQHandler(void)
{
	LPC_CCAN_API->isr();
}


void CAN_Init(int baudrate)
{
	uint32_t CanApiClkInitTable[2];
   	/* Publish CAN Callback Functions */
   	CCAN_CALLBACKS_T callbacks =
   	{
   		CAN_rx,
   		CAN_tx,
   		CAN_error,
   		NULL,
   		NULL,
   		NULL,
   		NULL,
   		NULL,
   	};
   baudrateCalculate(baudrate, CanApiClkInitTable);

   LPC_CCAN_API->init_can(&CanApiClkInitTable[0], TRUE);
   /* Configure the CAN callback functions */
   LPC_CCAN_API->config_calb(&callbacks);
}

void CAN_SetupRx(void){
/* Configure message objects
 * 1 = 581,
 * 2 = 281,
 * 3 = 580 - 5FF,
 * 4 = 600 - 67F,
 * 31 = All the rest
 * Msg objects must be configured from most restrictive filter to least
 * The CAN hardware uses the first object that matches the incoming message*/

	msg_obj.msgobj = 1;
	msg_obj.mode_id = 0x581;
	msg_obj.mask = 0xFFF;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);

	msg_obj.msgobj = 2;
	msg_obj.mode_id = 0x281;
	msg_obj.mask = 0xFFF;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);

	msg_obj.msgobj = 3;
	msg_obj.mode_id = 0x600;
	msg_obj.mask = 0xF80;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);

	msg_obj.msgobj = 4;
	msg_obj.mode_id = 0x580;
	msg_obj.mask = 0xF80;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);

	msg_obj.msgobj = 31;
	msg_obj.mode_id = 0x000;
	msg_obj.mask = 0x000;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);
}

void CAN_Enable(void)
{
   /* Enable the CAN Interrupt */
	NVIC_EnableIRQ(CAN_IRQn);
}

void CAN_Disable(void)
{
	// Disable CAN interrupt
	NVIC_DisableIRQ(CAN_IRQn);
}

void CAN_Send(char* msg)
{
	char can_msg[10];
	hextobytes(msg, can_msg, 20);
	msg_obj.msgobj  = 0;
	msg_obj.mode_id = can_msg[0]<<8 | can_msg[1];
	msg_obj.mask    = 0x0;
	msg_obj.dlc     = 8;
	msg_obj.data[0] = can_msg[2];
	msg_obj.data[1] = can_msg[3];
	msg_obj.data[2] = can_msg[4];
	msg_obj.data[3] = can_msg[5];
	msg_obj.data[4] = can_msg[6];
	msg_obj.data[5] = can_msg[7];
	msg_obj.data[6] = can_msg[8];
	msg_obj.data[7] = can_msg[9];
	LPC_CCAN_API->can_transmit(&msg_obj);

}

void CAN_Get_msg(char* dest)
{
	int i;
	i = CAN_Write_Idx -1; // the index of the last message received
	arraycpy(Msg_Stack[i], dest, 10);
	return;
}

int CAN_set_opt(char* message)
{
switch(message[2])
					{
						case 'D': // D for Disable
							CAN_Disable();
							process_can_msgs = false;
							return 1;
						break;

						case 'E': // E for Enable
							CAN_Enable();
							process_can_msgs = true;
							return 1;
						break;

						case 'F':// F for Filter selection
							switch(message[3])
							{
								case 'A':// A for All
									allmsgs = true;
									msg581only = false;
									return 1;
								break;
								case 'S':// S for SDO
									allmsgs = false;
									msg581only = false;
									return 1;
								break;
								case '5':// 5 for 581
									allmsgs = false;
									msg581only = true;
									return 1;
								break;
								default:
									//Serial_Print(BAD_OPT, MSG_LEN);
								break;
							}
						break;

						default:
							//Serial_Print(BAD_OPT, MSG_LEN);
						break;
					}
	return 1;
}

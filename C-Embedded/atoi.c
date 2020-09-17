/*
 * atoi.c
 *
 *  Created on: Oct 24, 2013
 *      Author: HollyWynn
 */

#include "atoi.h"
#include "itoa.h"
#include "string.h"

int hatoi(char c){
	if ( c > 70){
		return(c - 87);
	}
	else if ( c > 57){
		return(c - 55);
	}
	else{
		return(c - 48);
	}
}

int hextobyte(char* str){
	int highnibble = hatoi(str[0]);
	int lownibble = hatoi(str[1]);
	return highnibble<<4 | lownibble;
}

void hextobytes(char* inbuff, char* outbuff, int len){
	// outbuff must be size len/2, len must be power of 2
	char substr[2];
	for(int i = 0; i < len; i += 2){
	    substr[0] = inbuff[i];
	    substr[1] = inbuff[i+1];
	    outbuff[i/2] = hextobyte(substr);
	    }
}

void bytestohex(char* inbuff, char* outbuff, int len){
	// outbuff must be 2x len
	char ch;
	char tmp[4];
	for (int i = 0; i < len; i++){
		itoa(inbuff[i], tmp, 16);
		if (strlen(tmp) < 2){
			ch = tmp[0];
			tmp[0]= '0';
			tmp[1] = ch;
		}
		outbuff[(i*2)] = tmp[0];
		outbuff[(i*2)+1] = tmp[1];
	}
}

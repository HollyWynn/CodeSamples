## Test Script for DS402_Updater
## Emulates an DS402 CanOpen node for the purposes of the updater
## Used in place of CAN_Driver.py
## Started 2/26/13


import time
from ctypes import *
import binascii
## Represents a CAN message
##
class CANMsg (Structure):
    """
    Represents a CAN message
    """
    _fields_ = [ ("ID",      c_ulong),          # 11/29-bit message identifier
                 #("MSGTYPE", TPCANMessageType), # Type of the message Not Used Here
                 ("LEN",     c_ubyte),          # Data Length Code of the message (0..8)
                 ("DATA",    c_ubyte * 8) ]     # Data of the message (DATA[0]..DATA[7])

logfile = open('Log.TXT', 'w')
binfile = open('Firmware.BIN', 'wb')

## Placeholder for real function in CAN_Driver.py
##
def connect(): 
   print("Connected")
        
        
## Placeholder for real function in CAN_Driver.py
##
def write(CANMsg):
    return SDO_Test(CANMsg)
    
mock_CanOpen_ObjDict = { 0x1018+0x04: (5471043, 4),
                         0x4000+0x01: (4, 1),
                         0x4000+0x05: (3000, 4),
                         0x4001+0x04: (1075, 4),
                         0x4001+0x05: (2300, 2),
                         0x4002+0x04: (1050, 4),
                         0x1000+0x00: (0x10000000, 4),
                         0x1F56+0x01: (0x333403EA, 4)
                         }
    

## Message Parser to get data out of CAN Message
##
def log(msg, logfile):
    '''(CANMsg) -> 4 Tuple (CAN_ID, Object Index, Sub Index, Data)
    writes the CANMsg to the log file object specified. Expects the file to already be open for writing.
    '''
    data = ''
    id = msg.ID
    header = msg.DATA[0]
    if ((header >> 5) == 0 ): # this is a segment
        for i in range(1, 8):
            data = data + chr(msg.DATA[i]).encode('hex')
        logfile.write(hex(id) + ' ' + hex(header) + ' ' + data + '\n')
    else: # this is an expedited msg
        index = (msg.DATA[2] << 8) | msg.DATA[1] 
        sub_index = msg.DATA[3]
        data = msg.DATA[4] | (msg.DATA[5] << 8) | (msg.DATA[6] << 16)| (msg.DATA[7] << 24)   
        logfile.write(hex(id) + ' ' + hex(header) + ' ' + hex(index) + ' ' + hex(sub_index) + ' ' + hex(data) + '\n')
    logfile.flush()
    return
            
    
    
    
## CAN response message creator
##
def SDO_Test(in_CANMsg): 
    '''(CANMsg) -> CANMsg
    Takes a CANMsg, evaluates it, and returns an appropriate response CANMsg indicating the response.
    Also logs incoming and outgoing messages as well as dumping segmented transfer data to a file
    '''
    
    ##Setup the out_CANMsg
    ##Default values will changed if needed when the message is handled
    out_CANMsg = CANMsg()
    out_CANMsg.ID = 0x581 
    out_CANMsg.LEN = 8 # CANOpen messages are always 8 bytes
    out_CANMsg.DATA[0] = 0
    out_CANMsg.DATA[1] = 0
    out_CANMsg.DATA[2] = 0
    out_CANMsg.DATA[3] = 0
    out_CANMsg.DATA[4] = 0
    out_CANMsg.DATA[5] = 0
    out_CANMsg.DATA[6] = 0
    out_CANMsg.DATA[7] = 0
    
    ## Parse the "Outbound" CANMsg to make life easier
    id = in_CANMsg.ID
    header = in_CANMsg.DATA[0]
    index = (in_CANMsg.DATA[2] << 8) | in_CANMsg.DATA[1] 
    sub_index = in_CANMsg.DATA[3]
    data = in_CANMsg.DATA[4] | (in_CANMsg.DATA[5] << 8) | (in_CANMsg.DATA[6] << 16)| (in_CANMsg.DATA[7] << 24)
    
    ## Log the "Outbound CANMsg
    log(in_CANMsg, logfile)
    
    ## Explode the header into its bitfields and extract the data
    css = header >> 5
    e = (header & 0b00000010) >> 1
    s = (header & 0b00000001) 
    c = 0
    t = 0
    n = 0
    is_segmented = False
    is_expedited = False
    
    ## Check for message type to extract number of null bytes in message and message specific flags
    if ((s == 1) and (e == 1) and (css == 1)): # This is a write, size is indicated AND message is expedited.
        n = (header & 0b00001100) >> 2
        num_valid_bytes = 4-n
        is_expedited = True
        
    if ((s == 1) and (e == 0) and (css == 1)): #This is a write,  size is indicated AND message is start of segmented transfer
        # n is in the data in CanOpen format (high byte last)
        n = in_CANMsg.DATA[4] | (in_CANMsg.DATA[5] << 8) | (in_CANMsg.DATA[6] << 16)| (in_CANMsg.DATA[7] << 24)
        num_valid_bytes = 4-n
        is_expedited = True
        
    if (css == 0): # This is a segment
        n = (header & 0b00001110) >> 1
        c = (header & 0b00000001)  # Last segment flag
        t = (header & 0b00010000) >> 4 # Toggle bit
        num_valid_bytes = 7-n
        is_segmented = True
    
    
    is_read = (css == 2)
    is_write = (css == 1)
    is_last_seg = (c == 1)
    contains_null_bytes = (n > 0)
    
    
    ## Request next segment
    if (css == 3): 
        # This should not be encountered in this application.
        # Print an error and return the input msg for logging.
        print("ERR Encountered Unexpected CanOpen Header")
        return in_CANMsg
    
    ## Write Request
    elif (css == 1): 
        #create write response 
        # respond with index, sub index, and empty (default) data
        # segmented write init and expedited write have the same leading 4 bits
        out_CANMsg.DATA[0] = 0b01100000
        out_CANMsg.DATA[1] = in_CANMsg.DATA[1]
        out_CANMsg.DATA[2] = in_CANMsg.DATA[2]
        out_CANMsg.DATA[3] = in_CANMsg.DATA[3]
        #DATA 4-7 are 0 (default) unless seg write init
        
    
    ## Read Request
    elif (css == 2): 
        #create read response
        #requires mock object dictionary 
        out_CANMsg.DATA[0] = 0b01000011 | ((4 - mock_CanOpen_ObjDict[index+sub_index][1]) << 2) # Number of null bytes for expedited transfer (MAX 4 data bytes)
        out_CANMsg.DATA[1] = in_CANMsg.DATA[1]
        out_CANMsg.DATA[2] = in_CANMsg.DATA[2]
        out_CANMsg.DATA[3] = in_CANMsg.DATA[3]
        #Reverse data byte order
        data = mock_CanOpen_ObjDict[index+sub_index][0]
        out_CANMsg.DATA[4] = (data << 24)>> 24 #data low byte
        out_CANMsg.DATA[5] = (data << 16)>> 24 #data byte 1
        out_CANMsg.DATA[6] = (data << 8 )>> 24 #data byte 2
        out_CANMsg.DATA[7] =  data >> 24 #data high byte
    
    ## Must be a segmented transfer segment (CSS == 0)
    else: #This must be a segmented transfer segment
        #create segment write response
        #Make sure toggle bit is toggled
        out_CANMsg.DATA[0] = (header & 0b00010000) | 0b00100000 ##extracts toggle bit from original header and adds SCS header
        #everything else is 0 (default)
        # logs the data from the segmented transfer in the binfile
        
        for i in range(1, num_valid_bytes + 1):
            binfile.write(chr(in_CANMsg.DATA[i]))
        binfile.flush()
        
    ## Log the "Inbound" CANMsg
    log(out_CANMsg, logfile)
    
    

    return out_CANMsg
        
    
    
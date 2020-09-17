### Downloader module

def downloader(in_file, com_port, is_widelane):
    
    
    

    
    
    
    ## Wide lane values
    widelane_firmware_filename = "wide.s"
    widelane_checksum = [0xDF, 0xE5]
    
    
    ## Narrow lane values
    narrowlane_firmware_filename = "narrow.s"
    narrowlane_checksum = [0x00, 0x00]
    
    
    ## create read and write buffers
    ##2 dimensional array, PAGE x DATA
   
    #print("Starting.....")
    #print("Creating Write Buffer..."),
    write_buf = [] #empty list
    for i in range(0,1040):
        write_buf.append(bytearray(256))
        #print(str(i)),
    for row in write_buf:
        for i in range(0, 256):
            row[i] = '\xFF'
    print("Write Buffer Created.")

    #write_buf = {}
    #read_buf = {}
    
    
    
    ## Cheating by using the calculated checksum from a different flasher for  wide lane
    if is_widelane:
        #print("Flashing Firmware for wide lane.")
        load_S_file(widelane_firmware_filename, write_buf)
        #print(" Done.")
        write_buf[16][0] = widelane_checksum[0]
        write_buf[16][1] = widelane_checksum[1]
        #print("Using pre-calculated checksum for wide lane firmware."),
        
    else:
        #print("Flashing Firmware for narrow lane.")
        load_S_file(narrowlane_firmware_filename, write_buf)
        #print(" Done.")
        #print("Calculating CRC....")
        ## run CRC and attach CRCs to write buffer
        ## crc functions in cCRC.py
        mycrc = crc.cCRC(16, 0x1021, 0xFFFF, True, True, 0x0000)
        ## Get the first row of i (first two bytes are checksum) done so we don't need an if check on each byte
        str_file_checksum = hex(write_buf[16][0]) + hex(write_buf[16][1])[-2:]
        #print("CRC in file is " + str_file_checksum)
        for j in range(2, len(write_buf[16])):
            mycrc.next(write_buf[16][j])
        for i in range(17, len(write_buf)):
            for j in range(0, len(write_buf[i])):
                mycrc.next(write_buf[i][j])
        crc_value = mycrc.crc()
        #print("Calculated Checksum is " + hex(crc_value)),
        ## stick the CRC in the appropriate page (crc is bytes 1 and 2 of page 16)
        write_buf[16][0] = crc_value & 0x00FF
        write_buf[16][1] = (crc_value >> 8) & 0x00FF
    
    #print(" Done.")
    
    #print("Starting Update Process")
    ## set up and open serial port
    my_serial = serial.Serial(com_port, 9600, timeout = 3)
    my_serial.setRTS(False)
    time.sleep(.200)
    
    ## establish communication with bootloader
    reset_to_bl(my_serial)
    if not (init_bl(my_serial)):
        return False
    
    ## verify ID
    if not (verify_id(my_serial)):
        return False
    #print("Current Software Version: " + binascii.hexlify(get_version(my_serial)).decode("hex"))
    
    ## erase the flash
    unlock_blocks(my_serial)
    if not (erase_blocks(my_serial)):
        return False
    
    ## write new program data
    if not (write_program(my_serial, write_buf)):
        return False
    #print(('\b'*21) +  "%.2f" % (float(100)) + "% Complete." )
    #progressbar.set_fraction(1)
    
    ## verify new program data
    ##bl.read_program(my_serial, read_buf)
    #print("Software update complete. Resetting MBC..."),
    reset_normal(my_serial)
    #print(" Done.")
    #print("Exiting Updater, Have a nice day!")
    return True



    

def load_S_file(in_file, buffer):
    S_file = open(in_file, 'r')
    #print("Loading Selected S-Record: " + in_file ),
    for record in S_file:
        ## parse, error on lines not starting with "S"
        if not record[0] == 'S':
            #print("ERR: Invalid S record File.")
            return False
            
        ##Ignore start and end records
        if record[1] == '2': #this is a data record with 3 address bytes
            ## block into byte count,address, and data, strip checksum
            data_size = int(record[2:4], 16) -4
            base_address = (int(record[4:6], 16) * 0x10000) + (int(record[6:8], 16) * 0x100) + int(record[8:10], 16)
            data = record[10:10 + (data_size * 2)].decode('hex')
            ''' Integrity checks during testing, may not be needed
            if not len(data) == data_size:
                print(record)
                print(data.encode('hex'))
                print(record[10:10 + (data_size * 2)])
                print(data_size, len(data))
                exit()
                '''
            ## write to buffer
            
            #if page_address not in buffer:
                #buffer[page_address] = bytearray(256)
            for i in range(0, data_size):
                page_address = (base_address + i - 0x0C0000) / 0x100 + 16
                buffer[page_address][base_address + i & 0x0000FF] = data[i]
                #print(address, record, i, len(data), data_size)
    S_file.close()
    return True
    
    
## functions for communicating with the bootloader

## Bootloader Command strings
UNLOCK_ALL_BLOCKS = '\x75'
ERASE_UNLOCKED_BLOCKS = '\xA7\xD0'
VERIFY_ID = '\xF5\xDF\xFF\x0F\x07'
READ_SREG = '\x70' #read status register command 0x70
CLEAR_SREG = '\x50' #clear status register command 0x50
WRITE_PAGE = '\x41'
GET_VERSION = '\xFB'

## Bootloader baud rate values
B_9600 = '\xB0'
B_19200 = '\xB1'
B_38400 = '\xB2'
B_57600 = '\xB3'

DEFAULT_ID ="\x00\x00\x00\x00\x00\x00\x00"


"""
 //__________________________________________________________________________________________
        //Das Statusregister der MCU                                                                +
        //SRD1: 7 6 5 4 3 2 1 0                                                                     |
        //      | | | | | | | |->Reserved                                                           |
        //      | | | | | | |--->TimeOut(1) / Normal operation (0)                                  |
        //      | | | | | |----->ID-Check Bit 1 | 00: Not verified | 01: Verify mismatch            |
        //      | | | | |------->ID-Check Bit 2 | 10: Reserved     | 11: Verified                   |
        //      | | | |--------->CheckSum match bit (1=match, 0=mismatch)                           |
        //      | | |----------->Reserved                                                           |
        //      | |------------->Reserved                                                           |
        //      |--------------->Boot update complete bit (1=completed, 1=not update)               |
        //SRD0: 7 6 5 4 3 2 1 0                                                                     |
        //      | | | | | | | |->Reserved                                                           |
        //      | | | | | | |--->Reserved                                                           |
        //      | | | | | |----->Reserved                                                           |
        //      | | | | |------->Block status after program (1=Terminated in error)                 |
        //      | | | |--------->Program status (1=Terminated in error)                             |
        //      | | |----------->Erase status (1=Terminated in error)                               |
        //      | |------------->Reserved                                                           |
        //      |--------------->Write State Machine Status (1=Ready)                               |
        byte[] SRD0 = new byte[1];//                                                                |  
        byte[] SRD1 = new byte[1];//                                                                |                                                             
        //__________________________________________________________________________________________+
"""
## All Functions expect the serial_port to already be open
## this is due to a windows implementation causing the RTS line to toggle when the port is opened,
## this behavior resets the bootloader and is what screws up old versions of the flasher


def get_status(serial_port):
    ##Gets that status register value and returns the status
    serial_port.write(READ_SREG)
    SRD0 = serial_port.read() 
    SRD1 = serial_port.read() 
    serial_port.write(CLEAR_SREG)
    return (SRD0, SRD1)
    

def verify_id(serial_port):
    ## Send the ID and then checks to make sure it was verified. Returns True if ID is correct
    VERIFY_OK_MASK = 0b00001100
    serial_port.write(VERIFY_ID+DEFAULT_ID)
    SRD0, SRD1 = get_status(serial_port)
    #print(binascii.hexlify(SRD0),binascii.hexlify(SRD1))
    #print("ID Verify: OK.")
    return (ord(SRD1) & VERIFY_OK_MASK == VERIFY_OK_MASK)

def write_program(serial_port, write_buf):
    #print("Uploading New Program...")
    #sort a list of the keys in the dict(write_buff)
    WRITE_ERROR = 0x10
    #pages = write_buf.keys()
    #pages.sort()
    for i in range(16, len(write_buf)):
        if write_buf[i] != ('\xFF' * 256):
            address = 0xC0000 + (i-16) * 0x100
            address_lowbyte = (address >> 8) & 0xFF
            address_highbyte = (address >> 16) & 0xFF
            ## send COMMAND(1 byte) + ADDRESS(2 bytes) + data(256 bytes)
            serial_port.write(WRITE_PAGE + chr(address_lowbyte) + chr(address_highbyte) + write_buf[i])
            res = get_status(serial_port)
            while (((ord(res[0]) & 0x80) == 0)):
                time.sleep(.005)
                res = get_status(serial_port)
            if (ord(res[0]) &  WRITE_ERROR):
                #print("ERR: Error while programming flash.")
                return False
            ## Progress Indicator
            #if (i % 10) == 0:
        #print(('\b'*21) +  "%.2f" % ((len(write_buf) - (len(write_buf) - i)) / float(len(write_buf))*100) + "% Complete..." ),
        #progressbar.set_fraction((len(write_buf) - (len(write_buf) - i)) / float(len(write_buf)))
    #print("\nProgram download successful")
    return True

def read_program(serial_port, buffer):
    serial_port.write('\xFF')
    return 

def erase_blocks(serial_port):
    #print("Erasing Flash Blocks..."),
    WRITE_STATE_READY = 0x80
    WRITE_FAILED = 0x20
    ## erasing blocks apparently can take a while, change the serial port timeout
    old_timeout = serial_port.timeout
    serial_port.timeout = 11000
    serial_port.write(ERASE_UNLOCKED_BLOCKS)
    res = get_status(serial_port)
    timer = 0
    while (True):
        time.sleep(.010)
        timer += 10
        res = get_status(serial_port)
        if (ord(res[0]) & WRITE_STATE_READY == WRITE_STATE_READY):
            break
        elif (ord(res[0]) & WRITE_FAILED == WRITE_FAILED):
            serial_port.timeout = old_timeout
            #print("ERR: Block Erase Failure")
            return False
        elif timer > 11000:
            serial_port.timeout = old_timeout
            #print("ERR: Block Erase Timeout")
            return False
    serial_port.timeout = old_timeout
    #print(" Done.")
    return True

def unlock_blocks(serial_port):
    serial_port.write(UNLOCK_ALL_BLOCKS)
    return True

def reset_to_bl(serial_port):
    ## Set RTS for about 100mS
    serial_port.setRTS(True)
    time.sleep(.200)
    serial_port.setRTS(False)
    time.sleep(.200) # There is a possible lag between setting RTS and it actually happening du to system timer resolution
    return True

def reset_normal(serial_port):
    serial_port.setRTS(False)
    time.sleep(.2)
    serial_port.setRTS(True)
    serial_port.setRTS(False)
    return True
    
def init_bl(serial_port):
    # Tell bootloader we want to talk to it at 9600 baud
    # The bootloader should echo the setting
    serial_port.write(B_9600)
    res = serial_port.read(3)
    #print(res)
    if len(res) == 0:
        print("ERR: No Response From Bootloader!")
        return False
    
    if not res[0] == B_9600:
        print("ERR: Unexpected Response From Bootloader!")
        print(binascii.hexlify(res[0]), binascii.hexlify(B_9600))
        return False    
    
    for i in range(0,16):
        serial_port.write('\x00')
        time.sleep(.015) #write 0x00 16 times in >15mS intervals # I have no idea why, I think we're just waiting for baud gen sync.

    res = serial_port.read() #// now we check again that the bootloader has echoed 0xB0
    if len(res) == 0:
        print("ERR: No Response From Bootloader After Sync!")
        return False
    if not res[0] == B_9600:        
        print("ERR: Unexpected Response From Bootloader After Sync!")
        print(binascii.hexlify(res[0]), binascii.hexlify(B_9600))
        return False  
    #// set user baud rate if needed (should stay at 9600 to avoid gremlins)
    return True

def get_version(serial_port):
    ## get 8 bytes version id
    serial_port.write(GET_VERSION)
    time.sleep(.1)
    return serial_port.read(8)
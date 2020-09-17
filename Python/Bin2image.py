## Script to generate a memory image from a binary firmware file for CANOpen nodes.
## The files this script acts on have already been properly formatted. This will not work on random bin files.
"""
* Block structure as follows
Block 0:
OFFSET    PARAMETER    CONTENT
0x0000    Block Num       0
0x0004    Flash Addr      0
0x0008    Data Size[bytes]8
0x000C    Data       [reserved]
0x0010    Data       [reserved]
0x0014    Block CRC      CRC

Block 1-N:
OFFSET    PARAMETER    CONTENT
0x0000    Block Num     [1-N]
0x0004    Flash Addr   Dest Addr
0x0008    Data Size[bytes]n
0x000C    Data            n
0x000C+n    Block CRC      CRC
"""
## The processor used in this application has a max 512K flash
## create a 512KByte array, array indexes will be equivalent to flash addresses
flash_array = []
for i in range(0, 524289):
    flash_array.append(0x00)

flash_ptr = 0
data_size = 0

in_file = open("Firmware.bin", "rb")

block_zero = (24, 0)
in_file.read(24) # Block 0 is fixed, ignore it and don't put it in the flash image
## We can read 12 bytes in to a block to get the data size, then read data size + 4
done = False
blocks = [block_zero]
block_num = 0
total_size = 24
while (not done):
    block_head = in_file.read(12)
    if len(block_head) < 12:
        print("Done")
        break
    block_num = block_num + 1
    flash_ptr_str = block_head[-8:-4]
    data_size_str = block_head[-4:]
    
    
    data_size = 0
    for i in range(0, len(data_size_str)):
        data_size = data_size | (int(data_size_str[i].encode('hex'), 16) << (i * 8))
    flash_ptr = 0
    for i in range(0, len(flash_ptr_str)):
        flash_ptr = flash_ptr | (int(flash_ptr_str[i].encode('hex'), 16) << (i * 8))
    
    print("Block " + str(block_num) + " at Address " + str(flash_ptr) + ", " + str(data_size) + " bytes.")
    
    
    block_tail = in_file.read(data_size + 4)
    
    if (data_size > 8):
        for i in range(0, data_size):
            flash_array[flash_ptr + i] = block_tail[i]
    
output_file = open("Firmware_Image.bin", "wb")
for i in range(0, len(flash_array)):
    output_file.write(bytes(flash_array[i]))
output_file.close()    
    

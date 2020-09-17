## Parser to build bin file from recorded trace to find data anomalies

file = open("tracefile.txt", 'r')
out_file = open("firmware_compare.bin", 'wb')
last_line = ''
same_line_count = 0
for line in file:
    
    #print(line)
    # strip leading digit (frame number, can ID)
    start_offset = 0
    while line[start_offset].isdigit():
        start_offset = start_offset + 1
    # now we should have whitespace
    tmp = line[start_offset:].lstrip()
    # tmp should have len() >=21, we need the first 21 chars
    # strip trailing data (notes)
    line = tmp[0:30]
    #strip leading chars and all spaces to get a 7 byte string represented as hex
    line = line[5:28]
    line = line.replace(' ', '')
    #print(line)
    #exit()
    if ((line[0] == '1') or (line[0] == '0')): #we have a segment
        # we need to check for size too.
        
        num_valid_bytes = 7 - ((ord(line[:2].decode('hex')) & 0b00001110) >> 1) 
        print(num_valid_bytes)
        
        
        print(line)
        # try:
            # out_file.write(ord(line.decode('hex')))
        # except:
            # print(line)
            # exit()
            
        
        
        out_file.write(line[2:(num_valid_bytes*2)+2].decode('hex'))
        
                
file.close()
out_file.flush()
out_file.close()
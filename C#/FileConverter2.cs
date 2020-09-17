using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Windows;

namespace WpfApplication1
{
    class LogFileConverter2 : IDisposable
    { /* The LogFileConverter class provides methods for converting the binary
       *  log file to an ascii representation.
       *  This class handles V2 log files that have a new format to make parsing more reliable
       *  and limit possible errors
       * The convertFile() method converts the entire log and saves the output file to disk
       * The getNextMsg() method returns the next message from the binary log file
       */

        /*
         * The new protocol is as follows.

                   0               1               2               3               4               5               6               7
     7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                             time stamp                        |    x  |r|         cobid       |   BlckNo    |s|  x    | len   |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                            data                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    time stamp:   4 Bytes for TimeStamp
    s:            Flag for Special telegramms. => ASCII-Text in the data section („rev“ - revision ; „bov“ – buffer overrun …)
    BlckNo:           
    cobid:        CAN-Identifier
    r:            Flag for the RTR-Bit
    len:          number of Bytes in data section (0-8)
    data:         data section. Contains CAN data or ASCII if special bit is set

         * */
        private FileStream inputFS;
        private StreamWriter blockStatsSW;
        private StreamWriter errorStatsSW;
        private StreamWriter rawMsgsSW;
        private bool ignoreFormatErrors = false;
        private bool ignoreTimestampErrors = false;
        private bool cancelConversion = false;

        public bool relativeTimestamps = true;
        private bool bfirstTimestamp = false;
        private bool fixTimestamps = true;

        public int errorCounter = 0;

        public int lastTimestamp = 0;
        private int lastNewTimestamp = 0;
        private int timestampOffset = 0;
        public int firstTimestamp = 0;

        private int blockCount = 0;
        private int blockNum = 0;
        private int lastBlockNum = 0;
        private int blockMsgCount = 0;

        public int msgCount = 0;

        

        private string errorDetail = "";

        public LogFileConverter2(string inputFilePath)
        {
            inputFS = new FileStream(inputFilePath, FileMode.Open);
            blockStatsSW = new StreamWriter("blockstats.txt");
            errorStatsSW = new StreamWriter("errorstats.txt");
            rawMsgsSW = new StreamWriter("raw.txt");

        }

        // converts the entire log and writes to an output file without processing messages
        public void convertFile(string outputFilePath)
        {
            Encoding ascii = Encoding.ASCII;
            using (StreamWriter outputSW = new StreamWriter(outputFilePath, false, ascii))
            {
                string outputString = getNextMsg();
                while (!(outputString == ""))
                {
                    outputSW.Write(outputString + "\n");
                    outputString = getNextMsg();
                }

            }


        }


        private string getSMTextError(byte[] msg, int idx)
        {
            // TODO: FixME for new log format
            string retVal = BitConverter.ToString(msg).Replace("-", "").Substring(0, (idx * 2));
            if (retVal == "626F760D") // BOV, Buffer Overrun
            {
                retVal = "BOV: Buffer Overrun Detected! Some messages may be missing from trace!";
            }
            else // Unknown Error
            {
                retVal = "Unkown Error: " + BitConverter.ToString(msg).Replace("-", "").Substring(0, (idx * 2));
            }
            return retVal;
        }

        public string getNextMsg()
        {
            byte[] msg = new byte[16];

            int bytesRead = inputFS.Read(msg, 0, 16);
            if (bytesRead < 16)
            {
                inputFS.Close();
                blockStatsSW.Flush();
                blockStatsSW.Close();
                errorStatsSW.Flush();
                errorStatsSW.Close();
                rawMsgsSW.Flush();
                rawMsgsSW.Close();
                return ""; // Empty string indicates EOF
            }
            msgCount++;
            rawMsgsSW.Write(msgCount.ToString() + " " + BuildOldStyleMsg(msg) + "\n");
            // Check to make sure message is valid
            
            // Check message format
            if (!(CheckFormat(msg)))
            {
                errorCounter++;
                if (!(ignoreFormatErrors))
                {
                    MessageBoxResult res = MessageBox.Show("Message does not match required format!\n" + 
                        "Select Yes to continue, No to ignore further errors, or Cancel to stop processing " + 
                        "messages.\n" + BitConverter.ToString(msg).Replace("-", "") + "\n" + this.errorDetail, 
                        "Parse Error", MessageBoxButton.YesNoCancel);
                    if (res == MessageBoxResult.No)
                    {
                        ignoreFormatErrors = true;
                    }
                    if (res == MessageBoxResult.Cancel)
                    {
                        //cancelConversion = true;
                        return "";
                    }
                }

                //return "FORMAT0000000000000000000D"; // Breaks Timestamp extraction
                //return "0000000000000000000000000D"; // Pick something better for error indication
                //Temp treating like an OK message
            }

            CheckBlock(msg);

            if (cancelConversion)
            {
                if (errorCounter > 0)
                {
                    MessageBoxResult res = MessageBox.Show("There were errors encountered during processing. See error log for details.\nThe total number of errors found is " + errorCounter.ToString() + " errors.", "Parse Error", MessageBoxButton.OK);
                }
                return "";
            }
            else // we have a good message, lets see if the timestamp is OK
            {
                string retVal = BuildOldStyleMsg(msg);
                int timestamp = MsgUtils.getTimestampInt(retVal);
                if (!bfirstTimestamp)
                {
                    firstTimestamp = timestamp;
                    bfirstTimestamp = true;
                }
                if (!(timestamp >= lastTimestamp))
                {
                    errorCounter++;
                    string errorString = "\n\nError Count: " + errorCounter.ToString() +
                        "\n Message Number: " + msgCount.ToString() +
                        "\n This Timestamp: " + timestamp.ToString() + "\n High Timestamp: " +
                        lastTimestamp.ToString() + "\n Difference: " + (timestamp - lastTimestamp).ToString() +
                        "\n ID " + MsgUtils.getCobIdString(retVal);
                    errorStatsSW.Write(errorString);

                    if (!(ignoreTimestampErrors))
                    {
                        MessageBoxResult res = MessageBox.Show("Timestamp Error: Current timestamp is earlier " +
                        "than a previous timestamp!\nSelect Yes to continue, No to ignore further errors, or " +
                        "Cancel to stop processing messages.\n" + errorString,
                        "Parse Error", 
                        MessageBoxButton.YesNoCancel);

                        if (res == MessageBoxResult.No)
                        {
                            ignoreTimestampErrors = true;
                        }
                        if (res == MessageBoxResult.Cancel)
                        {
                            //cancelConversion = true;
                            retVal = "";
                        }
                    }
                }

                
                if (timestamp >= lastTimestamp)
                {
                    lastTimestamp = timestamp;
                }
                
                // Format message like the old format
                // Timestamp , cob ID, data bytes, data
                return retVal;
            }
        }
        private string BuildOldStyleMsg(byte[] newStyleMsg)
        {
            // 0C000000 0000 8203 0000 08 2708245D23000B00 0D
            string time = BitConverter.ToString(newStyleMsg, 0, 4).Replace("-", "");
            byte[] id = new byte[2];
            id[0] = newStyleMsg[4];
            id[1] = newStyleMsg[5];
            string cobID = BitConverter.ToString(id).Replace("-", "");
            byte[] blen = new byte[1];
            blen[0] = (byte)(newStyleMsg[6] & 0x0F);
            string len = BitConverter.ToString(blen).Replace("-", "");
            string data = BitConverter.ToString(newStyleMsg, 8, blen[0]).Replace("-", "");
            string newMsg =  time + "0000" + cobID + "0000" + len + data + "0D";

            return newMsg;
        }
        // BYTES 6 AND 7 APPEAR TO BE SWAPPED FROM THE DIAGRAM!!!!!
        // COMPENSATING HERE, SUBJECT TO CHANGE!!!
        private bool CheckFormat(byte[] msg)
        {
            
            bool byte4bit5check = ((msg[4] & 0x20) == 0);
            bool byte7bit6check = ((msg[6] & 0x40) == 0);
            this.errorDetail = "Byte 4 bit 5 check is " + byte4bit5check.ToString() + "\n" +
                "Byte 7 bit 6 check is " + byte7bit6check.ToString() + "\n";
            return (((msg[4] & 0x20) == 0) && ((msg[6] & 0x40) == 0));
        }

        private bool CheckBlock(byte[] msg)
        {
            this.blockNum = msg[7] >> 2;
            if (!(this.blockNum == this.lastBlockNum))
            {
                
                this.blockStatsSW.Write("\n Block Count " + this.blockCount.ToString() + 
                    " Block # " + this.blockNum.ToString() + " Message Count " +
                    this.blockMsgCount.ToString() + "\n");
                this.blockCount += 1;
                this.lastBlockNum = this.blockNum;
                this.blockMsgCount = 1;
                

            }
            else
            {
                this.blockMsgCount++;
            }
            return (this.blockMsgCount >= 64);
        }



        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // dispose managed resources
                inputFS.Close();
                blockStatsSW.Close();
            }
            // free native resources
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

    }
}
    



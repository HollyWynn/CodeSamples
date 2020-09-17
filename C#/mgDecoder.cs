using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfApplication1
{
    class mgDecoder:Decoder
    {
        /* This class contains lookup table and data decoding functions
         * for the log file messages.
         */
        public mgDecoder() { }

       

        public string lastMGMsgData = "";
        public int position;
        public int velocity;
        public int temperature;



        public override int processMsg(string msg)
        {
            int cobId = MsgUtils.getCobIdInt(msg);
            int nodeId = MsgUtils.getNodeIdInt(msg);
            string data = MsgUtils.getDataString(msg);
            int retVal = 0;
            msgCount++;
            statusString = "";
            if (nodeId == 0x2)
            { 
                switch (cobId)
                {
                    
                    case 0x082:  // MG Error
                        {
                            var error = decodeErrorStatus(data);
                            statusString = "ERR: ";
                            statusString += error.Item1;
                            
                            errorCount++;
                            break;
                        }

                    case 0x182:  // MG TXPDO1 
                        {
                            //FIXME
                            //F72CC7030000820100000227100DUnknown
                            //FC2CC7030000820100000227000DUnknown
                            statusString = "MG TXPDO1";


                            break;
                        }
                    case 0x282:  // MG TXPDO2 
                        {
                            //FIXME
                            //F72CC7030000820200000827100000000000000DUnknown
                            //FC2CC7030000820200000827000000000000000DUnknown
                            statusString = "MG TXPDO2";


                            break;
                        }

                    case 0x402:  // MG RXPDO2 
                        {
                            //FIXME
                            //EF2CC703000002040000063F00000000000DUnknown
                            //7B3EC703000002040000063F00504F23000DUnknown
                            statusString = "MG RXPDO2";


                            break;
                        }

                    case 0x582:  // MG SDO Response
                        {
                            var SDO = decodeSDO(data);

                            if (SDO.Item2 == 0x3401)
                            {
                                temperature = SDO.Item3;
                                statusString = "TMP MG: ";
                                statusString += SDO.Item3.ToString();
                                break;
                            }
                            
                                statusString = "SDOR MG: ";
                                statusString += SDO.Item1;
                                statusString += SDO.Item3.ToString();
                            
                            break;
                        }

                    case 0x602:  // MG SDO Request
                        {
                            var SDO = decodeSDO(data);
                            statusString = "SDOT MG: ";
                            statusString += SDO.Item1;
                            statusString += SDO.Item3.ToString();
                            break;
                        }

                    case 0x382:  // Mode + Position
                        {
                            position = Int32.Parse(MsgUtils.hexSwap(data.Substring(4, 6)), System.Globalization.NumberStyles.HexNumber); ;
                            statusString = "POS MG: ";
                            statusString += position.ToString();
                            
                            break;
                        }

                    case 0x482:  // Mode + Velocity
                        {
                            velocity = Int32.Parse(MsgUtils.hexSwap(data.Substring(4, 8)), System.Globalization.NumberStyles.HexNumber); ;
                            statusString = "VEL MG: ";
                            statusString += velocity.ToString();

                            break;
                        }

                    case 0x702: // Motor Gateway Heartbeat
                        {
                            heartbeatCount++;
                            break;
                        }



                    default:
                        {

                            statusString = "Unknown";
                            break;
                        }
                }//End switch

                lastCobId = cobId;

            }// End if(nodeId == 0x1)
            else { retVal = 2; } //"Non-MG Message Sent To MG Processor"

            return retVal;



        }

        public static string[] MGP_LOOKUP =
        {
            "Motor Position"
            
        };

        public static string[] MGV_LOOKUP =
        {
            "Motor Velocity"

        };

        public static string[] MGT_LOOKUP =
        {
            "Motor Temperature"

        };
    }
}

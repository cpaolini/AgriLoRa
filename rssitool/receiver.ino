// Feather9x_TX
#include <SPI.h>
#include <RH_RF95.h>

 //Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Set frequency
#define RF95_FREQ 915.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
 
  Serial.begin(115200); 
  while (!Serial) {
    delay(1);
  }
 
  delay(100);
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("LoRa Radio Initialized & Set To "); Serial.println(RF95_FREQ);
  
  // You can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

  //Set default coding rate and spreading factor
  rf95.setCodingRate4(5);
  Serial.println("Coding rate set to 4/5");
  rf95.setSpreadingFactor(10);
  Serial.println("Spreading factor set to 10");
}
 
int16_t packetnum = 0;  // packet counter, we increment per transmission
uint8_t spreadingFactor = 10; // 7 - 11 valid
uint8_t codingRate = 5;  // 5 - 8 valid (correspond with 4/5, 4/6, 4/7, 4/8)
uint8_t prevSF = spreadingFactor;  //holds previous spreading factor for switching back
uint8_t prevCR = codingRate;  //holds previos coding rate for switching back
uint8_t successfulTX = 0;    //counts the number of successful tx's
uint8_t unsuccessfulRX = 0;  //counts the number of unsuccessful rx's
uint8_t search = 0;   //after a too many unsuccessful rxs, start hopping channels & searching.
uint8_t swap = 0;

void loop()
{ 
  delay(5000); //Delay for 5s
  Serial.println("Transmitting...");

  if(swap == 0)  //If swap flag is 0, send a regular packet request
  {
    char radiopacket[30] = "Packet Request #";
    itoa(packetnum++, radiopacket+16, 10);
    Serial.print("Sending "); Serial.println(radiopacket);
    radiopacket[29] = 0;
    delay(10);
    rf95.send((uint8_t *)radiopacket, 30);
  }
  else if(swap == 1) //If swap flag is 1, send "Swap"
  {
    char radiopacket[6] = "Swap";
    Serial.print("Sending "); Serial.println(radiopacket);
    radiopacket[5] = 0;
    delay(10);
    rf95.send((uint8_t *)radiopacket, 6);
  }

  



 
  //Wait for packet to send
  delay(10);
  rf95.waitPacketSent();
  
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  
  //If swap is 1, set swap to 0 (to not swap again on next TX) and change to the new spreading factor / coding rate
  if(swap == 1)
  {
    swap = 0 ;
    spreadingFactor = spreadingFactor + 1;
    if (spreadingFactor > 10)
    { 
     spreadingFactor = 7;
     codingRate++;
     if (codingRate > 8)
      {
       codingRate = 5;
      }
      rf95.setCodingRate4(codingRate);
      Serial.print("Changed Coding Rate to: 4/");
      Serial.println(codingRate);
    }
    rf95.setSpreadingFactor(spreadingFactor);
    Serial.print("Changed Spreading Factor to: ");
    Serial.println(spreadingFactor);
  }
  
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
   {

      if(search == 1)   //If search is 1, we are looking for the receiver. When found, specify which SF and CR device was found at
      {
        search = 0;
        Serial.print("Found receiver at CF 4/ ");
        Serial.print(codingRate);
        Serial.print(" & SF ");
        Serial.println(spreadingFactor);
        
      }
      
      Serial.print("#[Got reply: ");
      Serial.print((char*)buf);
      Serial.print("]#");
      Serial.print("           #[RSSI: ");
      Serial.print(rf95.lastRssi());
      Serial.print(" dB");
      Serial.print("]#");
      Serial.print("             #[SNR: ");
      Serial.print(rf95.lastSNR());
      Serial.print(" dB");
      Serial.println("]#");
      successfulTX++;
      
      //After 10 successful transmits, swap settings
      if(successfulTX > 9)
      {
        swap = 1;
        successfulTX = 0;
      }
      delay(1000);
    }
    else
    {
      Serial.println("Receive failed");
    }

    
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
    unsuccessfulRX++;
    
    if (search == 1) //search for the receiver
    {
      Serial.println("#[Searching for receiver]#");
      unsuccessfulRX = 0; //reset unsuccessful RX
      spreadingFactor = spreadingFactor + 1; //increment spreading factor
      if (spreadingFactor > 10)
      { 
       spreadingFactor = 7;
       codingRate++;
       if (codingRate > 8)
        {
         codingRate = 5;
        }
        rf95.setCodingRate4(codingRate);
        Serial.print("#[Set Coding Rate to: 4/");
        Serial.print(codingRate);
        Serial.println("]#");
      }
      rf95.setSpreadingFactor(spreadingFactor);
      Serial.print("#[Changed Spreading Factor to: ");
      Serial.print(spreadingFactor);
      Serial.println("]#");

      Serial.println("#[Searching for receiver]#");

      delay(1000);
    }
    
    
    //After 10 unsuccessful receptions, swap back to previous spreading factor and coding rate and set search to 1 to begin looking for device
    if(unsuccessfulRX > 9 && search == 0)
    {
      Serial.println("Reverting back to previous SF and CR");
      unsuccessfulRX = 0;
      rf95.setSpreadingFactor(prevSF);
      rf95.setCodingRate4(prevCR);
      search = 1;
    }
    
    
    
  }
}

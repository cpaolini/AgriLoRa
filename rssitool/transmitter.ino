 // Feather9x_RX
#include <SPI.h>
#include <RH_RF95.h>
 
//Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3 //interrupt pin

//Set radio frequency
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
// LED Pin
#define LED 13

//Voltage Read Pin
#define VBATPIN A7




///
///This chunk of code is used to convert a float or double to a string
///
#include <stdlib.h>
#include <stdio.h>
#if 0
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}
#else
#include <string.h>
#include <stdlib.h>
char *dtostrf(double val, int width, unsigned int prec, char *sout)
{
  int decpt, sign, reqd, pad;
  const char *s, *e;
  char *p;
  s = fcvt(val, prec, &decpt, &sign);
  if (prec == 0 && decpt == 0) {
  s = (*s < '5') ? "0" : "1";
    reqd = 1;
  } else {
    reqd = strlen(s);
    if (reqd > decpt) reqd++;
    if (decpt == 0) reqd++;
  }
  if (sign) reqd++;
  p = sout;
  e = p + reqd;
  pad = width - reqd;
  if (pad > 0) {
    e += pad;
    while (pad-- > 0) *p++ = ' ';
  }
  if (sign) *p++ = '-';
  if (decpt <= 0 && prec > 0) {
    *p++ = '0';
    *p++ = '.';
    e++;
    while ( decpt < 0 ) {
      decpt++;
      *p++ = '0';
    }
  }    
  while (p < e) {
    *p++ = *s++;
    if (p == e) break;
    if (--decpt == 0) *p++ = '.';
  }
  if (width < 0) {
    pad = (reqd + width) * -1;
    while (pad-- > 0) *p++ = ' ';
  }
  *p = 0;
  return sout;
}
#endif
///
///This chunk of code is used to convert a float to a string
///




int debug = 1;
void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  pinMode(VBATPIN, INPUT);
  digitalWrite(RFM95_RST, HIGH);
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    //LoRa Initialization failed if in this loop
    while (1);
  }
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    //Setting Frequency failed if in this loop
    while (1);
  }
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

 //Set default coding rate and spreading factor
  rf95.setCodingRate4(5);
  rf95.setSpreadingFactor(10);
}

uint8_t spreadingFactor = 10;
uint8_t codingRate = 5;
uint8_t changeProperties = 0;


void loop()
{ 
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
 
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);

      //Check if the string received is "Swap", if it is, change spreading factor and/or coding rate
      if (buf[0] == 'S' && buf[1] == 'w' && buf[2] == 'a' && buf[3] == 'p')
      {
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
        }
        rf95.setSpreadingFactor(spreadingFactor);

        //Send back the message "Swapped" after swapping spreading factor and/or coding rate
        uint8_t data[] = "Swapped";
        rf95.send(data, sizeof(data));
        rf95.waitPacketSent();
    }

    else
    {
      //measure battery voltage to send
      float measuredvbat = analogRead(VBATPIN);
      measuredvbat *= 2;    // we divided by 2, so multiply back
      measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
      measuredvbat /= 1024; // convert to voltage

      
      // Send a reply
      uint8_t data[10];
      dtostrf(measuredvbat, 6, 2, (char*)data);
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();

      
    }  
      digitalWrite(LED, LOW);
    }
    else
    {
      //Receive failed if in this loop
    }
  }
}

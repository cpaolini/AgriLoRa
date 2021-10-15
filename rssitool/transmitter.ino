 // Feather9x_RX
#include <stdlib.h>
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

char *dtostrf(double val, signed char width, unsigned char prec, char *sout);

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

// ============================================================================
// Name        : dtostrf
// URL         : https://github.com/stm32duino/Arduino_Core_STM32/blob/main/cores/arduino/avr/dtostrf.c
// Author      : Cristian Maglie <c.maglie@arduino.cc>
// Version     : commit b626ed2 on Dec 21, 2020
// Copyright   : (c) 2013 Arduino.  All rights reserved.
// Description : Emulation for dtostrf function from avr-libc
// ============================================================================

char *dtostrf(double val, signed char width, unsigned char prec, char *sout)
{
  //Commented code is the original version
  /*char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;*/

  // Handle negative numbers
  uint8_t negative = 0;
  if (val < 0.0) {
    negative = 1;
    val = -val;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (int i = 0; i < prec; ++i) {
    rounding /= 10.0;
  }

  val += rounding;

  // Extract the integer part of the number
  unsigned long int_part = (unsigned long)val;
  double remainder = val - (double)int_part;

  if (prec > 0) {
    // Extract digits from the remainder
    unsigned long dec_part = 0;
    double decade = 1.0;
    for (int i = 0; i < prec; i++) {
      decade *= 10.0;
    }
    remainder *= decade;
    dec_part = (int)remainder;

    if (negative) {
      sprintf(sout, "-%ld.%0*ld", int_part, prec, dec_part);
    } else {
      sprintf(sout, "%ld.%0*ld", int_part, prec, dec_part);
    }
  } else {
    if (negative) {
      sprintf(sout, "-%ld", int_part);
    } else {
      sprintf(sout, "%ld", int_part);
    }
  }
  // Handle minimum field width of the output string
  // width is signed value, negative for left adjustment.
  // Range -128,127
  char fmt[129] = "";
  unsigned int w = width;
  if (width < 0) {
    negative = 1;
    w = -width;
  } else {
    negative = 0;
  }

  if (strlen(sout) < w) {
    memset(fmt, ' ', 128);
    fmt[w - strlen(sout)] = '\0';
    if (negative == 0) {
      char *tmp = (char *)malloc(strlen(sout) + 1);
      strcpy(tmp, sout);
      strcpy(sout, fmt);
      strcat(sout, tmp);
      free(tmp);
    } else {
      // left adjustment
      strcat(sout, fmt);
    }
  }

  return sout;
}

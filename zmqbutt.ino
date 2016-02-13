/*

ZeroMQ Arduino Ethernet Shield example

created 09 Feb 2015 
by Matt Ebert

You need an Ethernet Shield and (optionally) some sensors to be read on analog pins 0 and 1

This example performs the zmtp greeting and handshake for a REQ socket as documented at:

http://rfc.zeromq.org/spec:37

Ethernet is handled by the uIPEthernet library by (Norbert Truchsess) which uses 
the uIP tcp stack by Adam Dunkels.

 */

#include <SPI.h>
#include <UIPEthernet.h>
//#include <UIPServer.h>
#include <UIPClient.h>
#include "zmqduino.h"

const char reg_stream[] = {
  1,0,0,21,
  '[',// (1)
    '"','i','n','o','5','"',',',  // stream name (7)
    '{',// (1)
      '"','t','2','"',':','"','i','n','t','"', //(10)
    '}', // (1)
  ']' // (1)
};

char data[] = {
  1,0,0,20, // 4bytes + ',' + 4bytes
  '[',//(1)
    '"','i','n','o','5','"',',',//(7)
    '1',',',//(1)
    '{',//(1)
      '"','t','2','"',':','1','0','0','4',//(6)
    '}',// (1)
  ']'//(1)
};

uint8_t registerStream();

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};

// initialize the library instance:
EthernetClient client;

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip    (192,168,1,183);
//IPAddress server(192,168,1,213);
IPAddress server(128,104,160,150);
int reg_port = 5556;
int mes_port = 5557;

signed long next = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1000;  //delay between updates (in milliseconds)

uint8_t count = 0;

void setup() {
  Serial.begin(9600); //Turn on Serial Port
    //Ethernet.begin(mac, ip); //Initialize Ethernet
    
  delay(1000);
  Serial.println("Attempting DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }

  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP());

  // setup request socket
  if( zmq_connect( client, server, reg_port, REQ) ){
    client.stop();
    for(;;)
      ;
  }
  // register datastream with server
  Serial.println("registering...");
  if( registerStream() ){
    for(;;)
      ;
  }
  // disconnect from registering port
  Serial.println("disconnecting...");
  client.stop();
  
  delay(1000);
  
  // setup push socket
  Serial.println("set up PUSH...");
  if( zmq_connect( client, server, mes_port, PUSH) ){
    client.stop();
    for(;;)
      ;
  }
  Serial.println("Starting data...");

  // data is fixed length right now
  data[3] = sizeof(data)-4;
  delay(5000);
}

void loop() {
  Ethernet.maintain();

  // if timer has rolled over send data
  if( ((signed long)(millis()-next)) > 0 ){
    next = millis() + postingInterval;
    
//    uint32_t ms = millis();
//    uint32_t us = micros();
//    for( uint8_t i=0; i<4; i++){
//      data[15-i] = (char)ms;
//      data[28-i] = (char)ms;
//      ms = ms >> 4;
//      data[40-i] = (char)us;
//      us = us >> 4;
//    }

    uint8_t temp = count;
    for( uint8_t i=0; i<3; i++ ){
      data[23-i] = char(((uint8_t)'0') + (temp%10));
      temp /= 10;
    }
    count++;
    
    Serial.write((uint8_t*)data,sizeof(data));
    sendData(client,data,sizeof(data));
//    const char test[] = { 1,0,0,4,'t','e','s','t'};
//    Serial.write((uint8_t*)test,sizeof(test));
//    sendData(test,sizeof(test));
  }

  // check for incoming packet, do stuff if we need to
  uint8_t len = client.available();
  if( len ){
    len = zmq_readData( client, 0 ); // process header and get get actual mesg length
    //for(uint8_t i=0; i<len; i++){
      //Serial.print("0x");
      //Serial.print(zmq_buffer[i],HEX);
      //Serial.print(", ");
    //}
    //Serial.println();
  }
}

uint8_t registerStream(){
  //Serial.println("Sending stream registration");
  // TODO: check response from server
  sendData(client,reg_stream,sizeof(reg_stream));
  return 0;
}

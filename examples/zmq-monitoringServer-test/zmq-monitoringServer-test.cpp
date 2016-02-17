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

#include <Arduino.h>
#include <UIPEthernet.h>
#include <UIPClient.h>

#include "zmqduino.h"   // zmq interface
#include "monitoringServer.h" // monitoringServer interface

#define DHCP 1

uint8_t channels = 2;
uint8_t dataEntrySize = 5; // 16 bits ~> 65,000 -> 5 5 digits

char zmq_buffer[90]={0}; //!< buffer for zmq communication

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};

// initialize the library instance:
EthernetClient client;
ZMQSocket ZMQPush(client, zmq_buffer, PUSH);
DataPacket packet( channels, (char *)"cmon", 4, dataEntrySize, zmq_buffer + ZMQ_MSG_OFFSET );

// fill in an available IP address on your network here,
// for manual configuration:
//IPAddress ip    (192,168,1,183);
IPAddress ip    (10,128,226,195);
//IPAddress server(192,168,1,213);
//IPAddress server(128,104,160,150);
IPAddress server(10,128,226,183);
int reg_port = 5556;
int mes_port = 5557;

signed long next;// = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1000;  //delay between updates (in milliseconds)

int16_t counts[2] = {0};

void setup() {
  Serial.begin(115200); //Turn on Serial Port
  //Serial.begin(9600); //Turn on Serial Port
//  pinMode(49,OUTPUT);
//  digitalWrite(49,LOW);
//  digitalWrite(49,HIGH);
  PORTG |= (1<<0);  // set AMC7812 CS pin high if connected
    
  // set up ethernet chip
  delay(1000);
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
#if DHCP || UIP_UDP
  Serial.println(F("DHCP..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Faled DHCP"));
    for(;;)
      ;
  }
#else
  Serial.println(F("STATIC..."));
  Ethernet.begin(mac,ip);// cant use DHCP without using UDP
#endif

  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP());

  // setup request socket
  Serial.println(F("Setting up REQ socket"));
  ZMQSocket ZMQReq( client, zmq_buffer, REQ );
  int16_t len;
  do{
    uint8_t err = 0;
    len = -1;
    if( ZMQReq.connect( server, reg_port ) ){
      client.stop(); // TODO: deal with this better
      Serial.println("Cant connect to server");
      err = 1;
    }
    if(!err){
      // register datastream with server
      Serial.println(F("Registering data stream..."));
      len = packet.registerStream();
      ZMQReq.sendZMQMsg(len);
      len = ZMQReq.recv();
      if( len < 0 ){
        Serial.println(F("negative len returned"));
        client.stop();
      }
    }
  } while( len < 0 );
  // check that we got the expected response from the moitoring server
  if( len != 7 ){
    Serial.println(F("Invalid Response from server, length: "));
    Serial.println(len);
    for(;;)
      ;
  }
  // disconnect from registering port
  Serial.println(F("Disconnecting REQ socket..."));
  client.stop();
  
  delay(1000); // increase stability
  
  // setup push socket
  Serial.println(F("Setting up PUSH socket..."));
  if( ZMQPush.connect( server, mes_port ) ){
    Serial.println(F("shit"));
    client.stop(); // TODO: deal with this better
    for(;;)
      ;
  }
  Serial.println(F("Starting"));
  Serial.println(F("Data"));
  Serial.println(F("Stream"));
  delay(3000);
}

void loop() {
  Ethernet.maintain();

  // if timer has rolled over send data
  if( ((signed long)(millis()-next)) > 0 ){
    uint32_t ts = millis();
    next = ts + postingInterval;

    uint8_t len = packet.preparePacket( ts, counts );
    Serial.write((uint8_t*)(zmq_buffer+ZMQ_MSG_OFFSET),len);
    Serial.println();
    ZMQPush.sendZMQMsg(len);
    counts[1]++;
  }

  // check for incoming packet, do stuff if we need to
  uint8_t len = client.available();
  if( len ){
    len = ZMQPush.read(); // process header and get get actual mesg length
  }
  counts[0]++;
  if( counts[0] < 0 ){
    counts[0] &= 0x7fff;
  }
  if( counts[1] < 0 ){
    counts[1] &= 0x7fff;
  }
}

// normal arduino main function
int main(void){
  init();

  setup();

  for(;;){
    loop();
    if (serialEventRun) serialEventRun();
  }
  return 0;
}

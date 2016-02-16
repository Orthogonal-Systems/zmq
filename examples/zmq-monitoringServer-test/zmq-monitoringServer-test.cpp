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
//#include <SPI.h>
#include <UIPEthernet.h>
#include <UIPClient.h>

#include "zmqduino.h"   // zmq interface
#include "monitoringServer.h" // monitoringServer interface

uint8_t channels = 2;

char zmq_buffer[ZMQ_MAX_LENGTH] = {0}; //!< buffer for zmq communication

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};

// initialize the library instance:
EthernetClient client;
ZMQSocket ZMQPush(client, zmq_buffer, PUSH);
DataPacket packet( channels, "testy", 5, 5, zmq_buffer + ZMQ_MSG_OFFSET );

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip    (192,168,1,183);
//IPAddress server(192,168,1,213);
IPAddress server(128,104,160,150);
int reg_port = 5556;
int mes_port = 5557;

signed long next = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1000;  //delay between updates (in milliseconds)

int16_t counts[2] = {0};

void setup() {
  Serial.begin(115200); //Turn on Serial Port
    
  // set up ethernet chip
  delay(100);
  Serial.println("Attempting DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed DHCP");
    for(;;)
      ;
  }
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP());

  // setup request socket
  ZMQSocket ZMQReq( client, zmq_buffer, REQ );
  if( ZMQReq.connect( server, reg_port ) ){
    client.stop(); // TODO: deal with this better
    for(;;)
      ;
  }
  // register datastream with server
  Serial.println("registering...");
  uint8_t len = packet.registerStream();
  ZMQReq.sendZMQMsg(len);
  len = ZMQReq.recv();
  if( len < 0 ){
    client.stop();
    for(;;)
      ;
  }
  // check that we got the expected response from the moitoring server
  if( len != 6 ){
    Serial.println("wrong registration response len: ");
    Serial.print(len);
    for(;;)
      ;
  }
  // disconnect from registering port
  Serial.println("disconnecting...");
  client.stop();
  
  delay(1000); // increase stability
  
  // setup push socket
  Serial.println("set up PUSH...");
  if( ZMQPush.connect( server, mes_port ) ){
    client.stop(); // TODO: deal with this better
    for(;;)
      ;
  }
  Serial.println("Starting data...");
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
    //for(uint8_t i=0; i<len; i++){
      //Serial.print("0x");
      //Serial.print(zmq_buffer[i],HEX);
      //Serial.print(", ");
    //}
    //Serial.println();
  }
  counts[0]++;
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

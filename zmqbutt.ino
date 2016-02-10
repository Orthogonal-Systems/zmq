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

#define TX_PACKET_MAX_SIZE 64

const uint8_t zmq_maj_ver = 3;
const uint8_t zmq_min_ver = 0;

//const char packetLength[] = "len: ";

enum zmq_soc { REQ, PUSH };

const char partial_greeting[] = {  // 11 bytes
  0xff,0,0,0,0,0,0,0,0,0x7f,zmq_maj_ver
};

const char remaining_greeting[] = { // 53 bytes
  zmq_min_ver,'N','U','L','L',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const char handshake_REQ[] = {
  4, 38, // cmd flag, length (after these two bytes...)
  5, 'R','E','A','D','Y', // command-size, command-body
  // begin metadata
  // declare request socket
  11, 'S','o','c','k','e','t','-','T','y','p','e', // property name len, property name
  0,0,0,3,'R','E','Q', // 4byte size of value, socket type (request socket)
  // declare identity
  8,'I','d','e','n','t','i','t','y',
  0,0,0,0
};

const char handshake_PUSH[] = {
  4, 26, // cmd flag, length (after these two bytes...)
  5, 'R','E','A','D','Y', // command-size, command-body
  // begin metadata
  // declare request socket
  11, 'S','o','c','k','e','t','-','T','y','p','e', // property name len, property name
  0,0,0,4,'P','U','S','H', // 4byte size of value, socket type (push socket)
};

// register stream with monitoring server
//const char reg_stream[] = {
//  1,0,0,36,
//  '[',// (1)
//    '\"','i','n','o','2','\"',',',  // stream name (7)
//    '{',// (1)
//      '\"','t','o','y','1','\"',':','\"','i','n','t','\"',',', // (13)
//      '\"','t','o','y','2','\"',':','\"','i','n','t','\"', // (12)
//    '}', // (1)
//  ']' // (1)
//};
//
//char data[] = {
//  1,0,0,44, // 4bytes + ',' + 4bytes
//  '[',//(1)
//    '\"','i','n','o','2','\"',',',' ',//(7)
//    '1','4','5','5','0','8','3','4','7','7',',',' ',//(5)
//    '{',//(1)
//      '\"','t','o','y','1','\"',':',' ','4',',',' ',//(10)
//      '\"','t','o','y','2','\"',':',' ','1',//(9)
//    '}',// (1)
//  ']'//(1)
//};

const char reg_stream[] = {
  1,0,0,21,
  '[',// (1)
    '\"','i','n','o','5','\"',',',  // stream name (7)
    '{',// (1)
      '\"','t','2','\"',':','\"','i','n','t','\"', //(10)
    '}', // (1)
  ']' // (1)
};

char data[] = {
  1,0,0,20, // 4bytes + ',' + 4bytes
  '[',//(1)
    '\"','i','n','o','5','\"',',',//(7)
    '1',',',//(1)
    '{',//(1)
      '\"','t','2','\"',':','1','0','0','4',//(6)
    '}',// (1)
  ']'//(1)
};

uint8_t packetBuffer[TX_PACKET_MAX_SIZE];

void sendData(const char* msg, uint8_t len);
uint8_t exchangeGreeting();
uint8_t initiateHandshake(zmq_soc soc);
void registerStream();
uint8_t waitForServer(uint16_t timeout);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};

// initialize the library instance:
EthernetClient client;

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip    (192,168,1,183);
IPAddress server(192,168,1,213);
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

  //Serial.print("localIP: ");
  Serial.println(Ethernet.localIP());
  //Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  //Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  //Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());
  const String fail = "failed: ";
  int8_t err = 1;
  do {
    if( err < 0 ){
      Serial.print(fail);
      Serial.println(err);
      //Serial.println("retrying in 1 second...");
      delay(1000);
    }
    err = client.connect(server, reg_port);
  } while( err < 0 );
  const String conn = "Connected";
  Serial.println(conn);

  uint8_t success = 0;
  uint8_t ret;
  // send greeting to server, and check stuff
  do{
    ret = exchangeGreeting();
    if( ret ){
      //if( ret != 6 ){ //dhcp is fucky if I dont leave that stuff in
        Serial.print(fail);
        Serial.println(ret);
      //} else {
      //  success = 1;
      //}
    } else {
      success = 1;
    }
    if(!success){
      delay(1000);
    }
  } while(!success);
  
  // send handshake
  ret = initiateHandshake(REQ);
  if( ret ){
      Serial.print(fail);
      Serial.println(ret);
      while(1){}
  }
  // zmq push setup should now be complete
  
  // register datastream with server
  registerStream();
  // get confirmation
  if( waitForServer(1000) ){
    Serial.println("timeout");
  }
  uint8_t len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len); // expecting: [0, ""]
  //Serial.print(packetLength);
  Serial.println(len);
  Serial.write(packetBuffer,len);
  // disconnect from registering port
  client.stop();
  
  delay(1000);
  
  err = 1;
  do {
    if( err < 0 ){
      Serial.print(fail);
      Serial.println(err);
      //Serial.println("retrying in 1 second...");
      delay(1000);
    }
    err = client.connect(server, mes_port);
  
    success = 0;  
    if( err > 0 ){
      Serial.println(conn);
  
      // send greeting to server, and check stuff

      ret = exchangeGreeting();
      if( ret ){
        Serial.print(fail);
        Serial.println(ret);
      } else {
        success = 1;
      }
      if(!success){
        client.stop();
        delay(1000);
      }
    }
  } while(!success);
  
  // send handshake
  ret = initiateHandshake(PUSH);
  if( ret ){
      Serial.print(fail);
      Serial.println(ret);
      while(1){}
  }
  Serial.println(conn);
  //Serial.println("Starting data firehose...");
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
    sendData(data,sizeof(data));
//    const char test[] = { 1,0,0,4,'t','e','s','t'};
//    Serial.write((uint8_t*)test,sizeof(test));
//    sendData(test,sizeof(test));
  }

  // check for incoming packet, do stuff if we need to
  uint8_t len = client.available();
  if( len ){
    // if len > buffer size ...
    client.read(packetBuffer,len);
    //Serial.print(packetLength);
    Serial.println(len);
    for(uint8_t i=0; i<len; i++){
      Serial.print("0x");
      Serial.print(packetBuffer[i],HEX);
      Serial.print(", ");
    }
    Serial.println();
  }
}

uint8_t waitForServer(uint16_t timeout){
  // wait for response from server
  uint16_t i = 0;
  while( !client.available() ){
    if( i > timeout ){
      return 1;
    }
    i++;
  }
  return 0;
}

uint8_t exchangeGreeting(){
  sendData( partial_greeting, sizeof(partial_greeting) );

  if( waitForServer(1000) ){
    return 1;
  }

  uint8_t len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len);
  //Serial.print(packetLength);
  Serial.println(len);
  for(uint8_t i=0; i<len; i++){
    Serial.print("0x");
    Serial.print(packetBuffer[i],HEX);
    Serial.print(", ");
  }
  Serial.println();

  // now check that the partial greeting matches the spec
  uint8_t ret = 0;
  uint8_t alt_greeting = 0;
  // 11 bytes
  if( len != sizeof(partial_greeting) ){ 
    if( len != sizeof(partial_greeting)-1 ){
      ret = 2;
    } else {
      alt_greeting = 1;
    }
  }
  // signature
  if( !ret && ((packetBuffer[0]!=0xff) || (packetBuffer[9]!=0x7f)) ){
    ret = 3;
  }
  // major version
  if( !ret && (packetBuffer[10] != zmq_maj_ver) && !alt_greeting ){
    ret = 4;
  }
  memset(packetBuffer, 0, len); // clear data
  if(ret){
    return ret;
  }

  // if we made it this far the signature is ok
  //Serial.println("partial greeting recieved from server");
  // send rest of greeting
  sendData( remaining_greeting, sizeof(remaining_greeting) );

  if( waitForServer(1000) ){
    return 1;
  }

  len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len);
  //Serial.print(packetLength);
  Serial.println(len);
  Serial.write(packetBuffer,len);

  // now check that the partial greeting matches the spec
  ret = 0;
  // 53 bytes
  if( len != sizeof(remaining_greeting)+alt_greeting ){ 
    ret = 2;
  }
  // null mechanism
  if( !ret && ((packetBuffer[1+alt_greeting]!='N')||(packetBuffer[2+alt_greeting]!='U')||(packetBuffer[3+alt_greeting]!='L')||(packetBuffer[4+alt_greeting]!='L')) ){
    ret = 5;
  }
  memset(packetBuffer, 0, len); // clear data
  if(ret){
    return ret;
  }

  return 0;
}

// send data to server
void sendData(const char* msg, uint8_t len) {
  Serial.print("\nSending bytes: ");
  Serial.println(len);
  client.write((uint8_t*)msg, len);
}

uint8_t initiateHandshake(zmq_soc soc){
  //Serial.print("Sending NULL security handshake for ");
  switch(soc){
    case REQ:
      //Serial.println("REQ");
      sendData(handshake_REQ,sizeof(handshake_REQ));
      break;
    case PUSH:
      //Serial.println("PUSH");
      sendData(handshake_PUSH,sizeof(handshake_PUSH));
  }
  //Serial.println(" socket.");

  if( waitForServer(1000) ){
    return 1;
  }

  uint8_t len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len);
  //Serial.print();
  Serial.println(len);
  Serial.write(packetBuffer,len);

  return 0;
}

void registerStream(){
  //Serial.println("Sending stream registration");
  sendData(reg_stream,sizeof(reg_stream));
}

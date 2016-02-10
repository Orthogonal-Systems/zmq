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

const char partial_greeting[] = {  // 11 bytes
  0xff,0,0,0,0,0,0,0,0,0x7f,zmq_maj_ver
};

const char remaining_greeting[] = { // 53 bytes
  zmq_min_ver,'N','U','L','L',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const char handshake[] = {
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

// register stream with monitoring server
const char reg_stream[] = {
  1,0,0,36,
  '[',// (1)
    '\"','a','r','d','u','\"',',',  // stream name (7)
    '{',// (1)
      '\"','t','o','y','1','\"',':','\"','i','n','t','\"',',', // (13)
      '\"','t','o','y','2','\"',':','\"','i','n','t','\"', // (12)
    '}', // (1)
  ']' // (1)
};

uint8_t packetBuffer[TX_PACKET_MAX_SIZE];

void sendData(const char* msg, uint8_t len);
uint8_t exchangeGreeting();
uint8_t initiateHandshake();
void registerStream();

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

void setup() {
  Serial.begin(9600); //Turn on Serial Port
    //Ethernet.begin(mac, ip); //Initialize Ethernet
    
  delay(1000);
  
    if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      // no point in carrying on, so do nothing forevermore:
      for(;;)
        ;
    }

  Serial.print("localIP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());

  int8_t err = 1;
  do {
    if( err < 0 ){
      Serial.print("client failed to connect with error code: ");
      Serial.println(err);
      Serial.println("retrying in 1 second...");
      delay(1000);
    }
    err = client.connect(server, reg_port);
  } while( err < 0 );
  Serial.println("Client connected to server");

  uint8_t success = 0;
  uint8_t ret;
  // send greeting to server, and check stuff
  do{
    ret = exchangeGreeting();
    if( ret ){
      if( ret != 6 ){ //dhcp is fucky if I dont leave that stuff in
        Serial.print("greeting failed with code: ");
        Serial.println(ret);
      } else {
        success = 1;
      }
    } else {
      success = 1;
    }
    if(!success){
      delay(1000);
    }
  } while(!success);
  
  // send handshake
  ret = initiateHandshake();
  if( ret ){
      Serial.print("handshake failed with code: ");
      Serial.println(ret);
      while(1){}
  }
  // zmq push setup should now be complete
  
  // register datastream with server
  registerStream();
}

void loop() {
  Ethernet.maintain();

  // if timer has rolled over send data
//  if( ((signed long)(millis()-next)) > 0 ){
//    next = millis() + postingInterval;
//    Serial.write((uint8_t*)hello,sizeof(hello));
//    sendData(hello,sizeof(hello));
//    hello[sizeof(hello)-1]++;
//  }

  // check for incoming packet, do stuff if we need to
  uint8_t len = client.available();
  if( len ){
    // if len > buffer size ...
    client.read(packetBuffer,len);
    Serial.print("received packet of length: ");
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
  Serial.print("received packet of length: ");
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
  Serial.println("partial greeting recieved from server");
  // send rest of greeting
  sendData( remaining_greeting, sizeof(remaining_greeting) );

  if( waitForServer(1000) ){
    return 1;
  }

  len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len);
  Serial.print("received packet of length: ");
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
  // not as-server (NULL mechaism has no "server")
  if( !ret ){
    for( uint8_t i=5; i<22; i++ ){
      //if(packetBuffer[i+alt_greeting]){
      if(packetBuffer){
        ret = 6;
      }
    }
  }
  memset(packetBuffer, 0, len); // clear data
  if(ret){
    return ret;
  }

  Serial.println("remaining greeting recieved from server");
  Serial.print("zmq version is: ");
  Serial.println(packetBuffer[0+alt_greeting]);
  Serial.println("using NULL mechanism");

  return 0;
}

// send data to server
void sendData(const char* msg, uint8_t len) {
  Serial.print("Sending message, bytes: ");
  Serial.println(len);
  client.write((uint8_t*)msg, len);
}

uint8_t initiateHandshake(){
  Serial.println("Sending NULL security handshake");
  sendData(handshake,sizeof(handshake));

  if( waitForServer(1000) ){
    return 1;
  }

  uint8_t len = client.available();
  // if len > buffer size ...
  client.read(packetBuffer,len);
  Serial.print("received packet of length: ");
  Serial.println(len);
  Serial.write(packetBuffer,len);

  return 0;
}

void registerStream(){
  Serial.println("Sending stream registration");
  sendData(reg_stream,sizeof(reg_stream));
}

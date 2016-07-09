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
#include <TimeLib.h>

#include "origin.h" // monitoringServer interface
#include "datapacket.h"

#define DHCP 0

uint8_t channels = 2;
uint8_t dataEntrySize = 5; // 16 bits ~> 65,000 -> 5 5 digits
uint8_t useFractionalSecs = 1;

char buffer[256]={0}; // communication buffer

// initialize the library instance:
EthernetClient client;

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip    (192,168,1,100);
//IPAddress ip    (10,128,226,195);
//IPAddress server(192,168,1,213);
IPAddress data_server(128,104,160,150);
//IPAddress server(10,128,226,183);
int reg_port = 5556;
int mes_port = 5557;

// origin data server interface
Origin origin( client
    , data_server
    , reg_port
    , mes_port
    , (char *)"uCTOY"   // stream name
    , 5         // stream name length (max 10)
    , 1         // use fractional seconds
    , 2         // maximum channels
    , 2         // channels used
    , (char *)DTYPE_INT16
    , INT16_TYPE_SIZE
    , buffer
);

EthernetUDP Udp;
// address for ntp server sync
IPAddress ntp_server(128,104,160,150);
int ntp_port = 8888;  // local port to listen for UDP packets

// mega pin 13
uint8_t lastTrig = 1;

signed long next;// = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5000;  //delay between updates (in milliseconds)
uint8_t longerSyncPeriod = 0; // state tracker

int16_t counts[2] = {0};

void sendNTPpacket(IPAddress &address);
time_t getNtpTime();

void setup_ethernet(){
  // set up ethernet chip
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
#if DHCP && UIP_UDP // cant use DHCP without using UDP
  Serial.println(F("DHCP..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed DHCP"));
    for(;;)
      ;
  }
#else
  Serial.println(F("STATIC..."));
  Ethernet.begin(mac,ip);
#endif

  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP());
}

void setup_ntp(){
  // setup NTP sync service
  Udp.begin(ntp_port);
  Serial.println(F("Waiting for NTP sync"));
  setSyncInterval(600); // initial one 10 minutes to take care of most of the drift
  setSyncProvider(getNtpTime);
}

void register_stream(){
  // setup request socket
  Serial.println(F("Registering stream with server."));
  uint8_t err;
  do{
    err = origin.registerStream();
    if(err==ERR_SERVER_RESP){
      Serial.println(F("Server error."));
    }
    if(err==ERR_SERVER_LEN){
      Serial.println(F("Invalid Response length from server."));
    }
  } while( err );
  Serial.println(F("Disconnecting REQ socket..."));
}

void setup_data_stream(){
  Serial.println(F("Setting up data stream..."));
  origin.setupDataStream();
  Serial.println(F("Starting"));
  Serial.println(F("Data"));
  Serial.println(F("Stream"));
}


void setup() {
  // minimal SPI bus config (cant have two devices being addressed at once)
  digitalWrite(SS, HIGH); // set ENCJ CS pin high 
  digitalWrite(41, HIGH); // daughtercard CS pin needs to be high if a card is connected

  Serial.begin(115200); //Turn on Serial Port for debugging

  setup_ethernet();
  setup_ntp();
  register_stream();

  delay(1000); // increase stability
  setup_data_stream();
  delay(1000);
}

uint32_t nextTrig = 0;

void loop() {
  Ethernet.maintain();
  now();  // see if its time to sync

  // if we havent extended the time period yet,
  // and if there is a non-zero drift correction
  // then make the period longer
  if( (!longerSyncPeriod) & getDriftCorrection() ){
    Serial.println("syncing to network time.");
    setSyncInterval(72000);  // every 2 hours
    longerSyncPeriod=1;
  }
  
  uint8_t newTrig = 1;
  if( millis() > nextTrig ){
    newTrig = 0;
    nextTrig = millis() + postingInterval;
  }
  if( !lastTrig && newTrig ){  // trig on high to low trigger
    //Send string back to client 
    time_t ts = now(0); // timestamp at start (dont allow ntp sync), ms
    uint32_t ts_sec = toSecs(ts);
    uint32_t ts_fsec = toFracSecs(ts);
    origin.sendPacket( ts_sec, ts_fsec, counts );
    counts[1]++;
  }
  lastTrig = newTrig;

  // check for incoming packet, do stuff if we need to
  uint8_t len = client.available();
  if( len ){
    Serial.println("incoming packet");
    client.read((uint8_t*)buffer, len);
    Serial.write(buffer, len);
  }

  counts[0] = client.connected();  // for debugging
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

/*-------- NTP code ----------*/
  
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(ntp_server);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 5000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      unsigned long fracSecs;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      fracSecs =  (unsigned long)packetBuffer[44] << 24;
      fracSecs |= (unsigned long)packetBuffer[45] << 16;
      fracSecs |= (unsigned long)packetBuffer[46] << 8;
      fracSecs |= (unsigned long)packetBuffer[47];
      return ((time_t)(secsSince1900 - 2208988800UL)<<32) + fracSecs;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

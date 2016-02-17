#include "monitoringServer.h"
#include <Arduino.h>

uint8_t DataPacket::addPreString(){
  buffer[0] = '[';
  buffer[1] = '"';
  memcpy(buffer+2, streamName, streamNameSize);
  buffer[streamNameSize+2] = '"';
  buffer[streamNameSize+3] = ','; 
  return streamNameSize+4;
}

// ready special packet in the buffer to send to sever to announce the data format
// returns length of packet in bytes
uint8_t DataPacket::registerStream(){
  uint8_t os = addPreString();
  buffer[os] = '{';
  for( uint8_t i=0; i<channels; i++ ){
    registerChannel(i,buffer+os+1+i*(6+CHANNEL_NAME_SIZE+INT_TYPE_SIZE));
  }
  buffer[registerSize-2] = '}';
  buffer[registerSize-1] = ']';
  return registerSize;
}

// ready the data packet in the buffer
// returns length of packet in bytes
uint8_t DataPacket::preparePacket( uint32_t timestamp, int16_t* data ){
  Serial.println(F("serious"));
  uint8_t os = addPreString();
  Serial.println(F("1"));
  int2charArray(timestamp, buffer+os);
  Serial.println(F("2"));
  buffer[os+TIMESTAMP_SIZE] = ',';
  Serial.println(F("3"));
  buffer[os+TIMESTAMP_SIZE+1] = '{';
  Serial.println(F("4"));
  for( uint8_t i=0; i<channels; i++ ){
    Serial.print("c");
    Serial.println(i);
    addChannelData( i, data[i], buffer+os+TIMESTAMP_SIZE+2+i*(4+CHANNEL_NAME_SIZE+dataEntrySize) );
  }
  Serial.println(F("5"));
  buffer[packetSize-2] = '}';
  buffer[packetSize-1] = ']';
  return packetSize;
}

// convert from int16 to char array with space padding
// pass pointer to the location in the buffer where data starts and fill it
// TODO: handle negatives
void DataPacket::int2charArray(int16_t x, char* buf){
  for(uint8_t i=dataEntrySize; i>0; i--){
    if( x == 0 ){
      buf[i-1] = ' ';
    } else {
      buf[i-1] = char(((uint8_t)'0') + (x%10));
      x /= 10;
    }
  }
}
void DataPacket::int2charArray(uint32_t x, char* buf){
  Serial.println(F("1."));
  for(uint8_t i=TIMESTAMP_SIZE; i>0; i--){
    Serial.print(F("1."));
    Serial.println(i);
    Serial.println(x);
    if( x == 0 ){
      buf[i-1] = ' ';
    } else {
      buf[i-1] = char(((uint8_t)'0') + (x%10));
      x /= 10;
    }
  }
}

uint8_t DataPacket::addChannelStr( uint8_t ch, char* buf ){
  if( ch < 10 ){
    buf[0] = ' ';
    buf[1] = '"';
    buf[2] = 'c';
    buf[3] = '0'+ch;
  } else {
    buf[0] = '"';
    buf[1] = 'c';
    buf[2] = '1';
    buf[3] = '0'+ch-10;
  }
  buf[4] = '"';
  buf[5] = ':';
  return 6;
}

void DataPacket::registerChannel( uint8_t ch, char* buf ){
  uint8_t os = addChannelStr( ch, buf );
  buf[os] = '"';
  buf[os+1] = 'i';
  buf[os+2] = 'n';
  buf[os+3] = 't';
  buf[os+4] = '"';
  buf[os+5] = ','; // if this is the last channel this will get overwritten
}

void DataPacket::addChannelData( uint8_t ch, int16_t x, char* buf ){
  uint8_t os = addChannelStr( ch, buf );
  int2charArray( x, buf + os );
  buf[os+dataEntrySize] = ','; // if this is the last channel this will get overwritten
}

#include "monitoringServer.h"
#include <Arduino.h>
#include <stdlib.h>

// ready special packet in the buffer to send to sever to announce the data format
// returns length of packet in bytes
uint8_t DataPacket::registerStream(char* dtype){
  uint8_t os = addPreString();
  for( uint8_t i=0; i<channels; i++ ){
    registerChannel(i,buffer+os+i*(2+CHANNEL_NAME_SIZE+DTYPE_STRING_MAXSIZE),dtype);
  }
  return registerSize;
}

// ready the data packet in the buffer
// returns length of packet in bytes
uint8_t DataPacket::preparePacket( uint32_t timestamp, int16_t* data ){
  return preparePacket( timestamp, 0, data);
}

uint8_t DataPacket::preparePacket( uint32_t timestamp, uint32_t fsTS, int16_t* data ){
  uint8_t os = int2charArray(streamKey, buffer);
  os += int2charArray(timestamp, buffer+os);
  os += int2charArray(fsTS, buffer+os); // add in second set of 4 bytes
  for( uint8_t i=0; i<channels; i++ ){
    addChannelData( i, data[i], buffer+os+i*INT16_TYPE_SIZE );
  }
  return packetSize;
}

uint8_t DataPacket::preparePacket( uint32_t timestamp, float* data ){
  return preparePacket( timestamp, 0, data);
}

uint8_t DataPacket::preparePacket( uint32_t timestamp, uint32_t fsTS, float* data ){
  uint8_t os = int2charArray(streamKey, buffer);
  os += int2charArray(timestamp, buffer+os);
  os += int2charArray(fsTS, buffer+os); // add in second set of 4 bytes
  for( uint8_t i=0; i<channels; i++ ){
    addChannelData( i, data[i], buffer+os+i*INT16_TYPE_SIZE );
  }
  return packetSize;
}

uint8_t DataPacket::addChannelStr( uint8_t ch, char* buf ){
  buf[0] = ',';
  if( ch < 10 ){
    buf[1] = ' ';
    buf[2] = '"';
    buf[3] = 'c';
    buf[4] = '0'+ch;
  } else {
    buf[1] = '"';
    buf[2] = 'c';
    buf[3] = '1';
    buf[4] = '0'+ch-10;
  }
  buf[5] = ':';
  return 6;
}

uint8_t DataPacket::addPreString(){
  memcpy(buffer, streamName, streamNameSize);
  return streamNameSize;
}

// dtype is the data type string, use defined values in header file
void DataPacket::registerChannel( uint8_t ch, char* buf, char* dtype ){
  uint8_t os = addChannelStr( ch, buf );
  memcpy(buf+os, dtype, DTYPE_STRING_MAXSIZE);
}

void DataPacket::addChannelData( uint8_t ch, int16_t x, char* buf ){
  int2charArray( x, buf );
}

void DataPacket::addChannelData( uint8_t ch, float x, char* buf ){
  float2charArray( x, buf );
}

#include "datapacket.h"
//#include <Arduino.h>
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

// read the message in the buffer of length len
// extract streamKey and return 0 if success, else return error code
uint8_t DataPacket::processStreamRegistration(uint8_t len){
  // the server response should be in the format of '[errorcode,streamKey]', where stream key is a uint32
  // that is used to identify the stream with the server.
  // error code is a char, 0 is no error
  if( buffer[1] != '0' ){
    return ERR_SERVER_RESP;
  } 
  if( len != 8 ){
    return ERR_SERVER_LEN;
  }
  streamRegistrationKey( buffer+3 );// store stream identifier
  return 0;
}

// ready the data packet in the buffer with no fractional second info
// returns length of packet in bytes
uint8_t DataPacket::preparePacket( uint32_t timestamp, int16_t* data ){
  return preparePacket( timestamp, 0, data);
}

// ready the data packet in the buffer with fractional second info
// returns length of packet in bytes
uint8_t DataPacket::preparePacket( uint32_t timestamp, uint32_t fsTS, int16_t* data ){
  memcpy(buffer, streamKey, STREAM_KEY_LENGTH);
  uint8_t os = STREAM_KEY_LENGTH;
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
  memcpy(buffer, streamKey, STREAM_KEY_LENGTH);
  uint8_t os = STREAM_KEY_LENGTH;
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
    buf[2] = 'c';
    buf[3] = '0'+ch;
  } else {
    buf[1] = 'c';
    buf[2] = '1';
    buf[3] = '0'+ch-10;
  }
  buf[4] = ':';
  return 5;
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

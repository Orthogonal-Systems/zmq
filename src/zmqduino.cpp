/*
 * ZeroMQ Arduino functions
 *
 *  Author: Matt Ebert
 *  2016/02
 *
 * Implements the ZMTP for a few basic sockets:
 *    REQ
 *    (REP?)
 *    PUSH
 *    (PULL?)
 *
 * ZMTP spec: http://rfc.zeromq.org/spec:37
 *
 * Most 8-bit MCU applications are going to be 1-1 sockets anyway, this is mostly
 * to trick zmq applications so they can be used.
 * Only NULL mechanism, and short messages are supported (max 256 bytes with overhead).
 *
 * Overhead
 *
 * All frames go into zmq_buffer, all message creation functions fill up the buffer 
 * and return the length of the message.
 *
 */

#define DEBUG 1

#ifdef DEBUG
#include "Arduino.h"
//const String fail = "Error: ";
//const String conn = "Connected";
//const String handshakestr = "Handshook";
#endif

#include <avr/pgmspace.h>
#include "UIPEthernet.h"
#include "UIPClient.h"
#include "zmqduino.h"

// message lengths
#define ZMQ_TGREET_LEN 64 // total greeting
#define ZMQ_PGREET_LEN 11 // partial greeting
#define ZMQ_RGREET_LEN 53 // remaining greeting
#define ZMQ_HS_PUSH_LEN 28 // push handshake
#define ZMQ_HS_REQ_LEN 40 // request handshake

#define ZMQ_CMD_FLAG 2
#define ZMQ_LONG_FLAG 1
#define ZMQ_MORE_FLAG 0

const char PUSHHandshake[] PROGMEM = {"\x04\x1C\x05READY\x0BSocket-Type\x00\x00\x00\04PUSH"};
const char REQHandshake[] PROGMEM = {"\x04\x28\x05READY\x0BSocket-Type\x00\x00\x00\03REQ\x08Identity\x00\x00\x00\x00"};

// zmq negotiate connection
int8_t ZMQSocket::connect( IPAddress host, uint16_t port ){
  int8_t err = 1;
  uint8_t success=0;
  uint8_t ret=0;
  do {
    if( err < 0 ){
#ifdef DEBUG
      Serial.print(F("Failed"));
      Serial.println(err);
#endif
      delay(1000);
    }

    err = client.connect(host, port);
    // client. connect returns negative error number, success is 1
    if( err > 0 ){
#ifdef DEBUG
      Serial.println(F("Connnected"));
#endif
  
      // send greeting to server, and check stuff
      ret = greet();
      if( ret ){
#ifdef DEBUG
        Serial.print(F("Failed"));
        Serial.println(ret);
#endif
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
  ret = sendHandshake();
  if( ret < 0 ){
#ifdef DEBUG
    Serial.print(F("Failed"));
    Serial.println(ret);
#endif
    return ret;
  }
#ifdef DEBUG
  Serial.println(F("handshook"));
#endif
  return 0;
}

int8_t ZMQSocket::greet(){
  // send partial greeting
  int16_t len = p_greeting();
  send( len );

  // recieve partial greeting, into zmq_buffer
  len = recv( 1 );
  if( len < 0 ){
    return len;
  }
  // verify major version (remember we are actually only getting 10 bytes from python code)
  uint8_t alternate_greeting = 0;
  switch(len){
    case ZMQ_PGREET_LEN-1:
      alternate_greeting = 1;
      break;
    case ZMQ_PGREET_LEN:
      if( buffer[ZMQ_PGREET_LEN] != majVer ){
        return 1;
      }
      break;
    default:
      return 2;
  }

  // send remaining greeting
  len = r_greeting();
  send( len );

  // recieve remaining greeting
  len = recv( 1 );
  if( len < 0 ){
    return len;
  }

  uint16_t shitty_chksum=0;
  for( uint8_t i=1; i<5; i++ ){
    shitty_chksum += buffer[i+alternate_greeting];
  }
  // checking null mechanism
  if( shitty_chksum != 'N'+'U'+'L'+'L' ){
    return 3;
  }
  // checking rest of null mechanism
  for( uint8_t i=6; i<20; i++ ){
    if( buffer[i+alternate_greeting] ){
      return 3;
    }
  }
  // we good
  return 0;
}

int8_t ZMQSocket::sendHandshake(){
  // send handshake
  uint8_t len = handshake();
  send( len );
  // recieve handshake
  uint16_t resp_len = recv( 1 );
  if( resp_len < 0 ){
    return resp_len;
  }
  // TODO: verify parameters
  return resp_len;
}

// send data to server (data is stored in buffer)
void ZMQSocket::send( uint8_t len ){
#ifdef DEBUG
  Serial.print("\nSending bytes: ");
  Serial.println(len);
  for(uint8_t i=0; i<len; i++){
    Serial.print(buffer[i],HEX);
    Serial.print(",");
  }
  Serial.println();
  Serial.write(buffer,len);
  Serial.println();
#endif
  client.write((uint8_t*)buffer, len);
}

int16_t ZMQSocket::recv(){
  return recv(0);
}

//  prepend message with ZMQ header then send it along
void ZMQSocket::sendZMQMsg( uint8_t msgLen ){
  buffer[0] = 1;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = msgLen;
  send( msgLen + 4 );
#ifdef DEBUG
  Serial.write(buffer+4,msgLen);
  Serial.println();
  for(uint8_t i=0; i<msgLen+4; i++){
    Serial.print(buffer[i],HEX);
    Serial.print(",");
  }
  Serial.println();
#endif
}

int16_t ZMQSocket::recv( uint8_t setup ){
  if( waitForServer() ){
#ifdef DEBUG
    Serial.println(F("timeout"));
#endif
    return -1;
  }
  return read( setup );
}

// negative -> error
// positive -> new bytes in zmq_buffer
int16_t ZMQSocket::read(){
  return read(0);
}

int16_t ZMQSocket::read( uint8_t setup ){
  uint8_t len = client.available();
#ifdef DEBUG
  Serial.print("\nRecv bytes: ");
  Serial.println(len);
#endif
  if( setup ){
    // TODO: read the len from a setup thing
    client.read((uint8_t*)buffer, len);
#ifdef DEBUG
    Serial.write(buffer,len);
    Serial.println();
#endif
    return len;
  }
  if( len < 4 ){
    client.read((uint8_t*)buffer, len);// clear buffer
    return -2;
  }
  client.read((uint8_t*)buffer, 4);
  // check that it isnt a long message, if it is clear it?
  if( buffer[0] & (1<<ZMQ_LONG_FLAG) ){
    return -3;
  }
  // check that there are at least the number of bytes we expect
  if( len - 4 < buffer[3] ){
    return -4;
  }
  // read the bytes sepecifed into the buffer
  len = buffer[3];
  client.read((uint8_t*)buffer, len);
  return len;
}

// send data to server, not from buffer
void ZMQSocket::send( const char* msg, uint8_t len ){
#ifdef DEBUG
  Serial.print("\nSending bytes: ");
  Serial.println(len);
#endif
  client.write((uint8_t*)msg, len);
}

uint8_t ZMQSocket::waitForServer(){
  // wait for response from server
  uint16_t i=0;
  while( !client.available() ){
    if( i > ZMQ_RESP_TIMEOUT_MS ){
      return 1;
    }
    i++;
    delay(1);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// ZeroMQ Message Transfer Protocol Stuff
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// exchange partial greeting
// signature + major version
uint8_t ZMQSocket::p_greeting(){
  buffer[0] = 0xff;
  // bits 1-8 are "dont care"
  for( uint8_t i=0; i<8; i++){
    buffer[i+1] = 0;
  }
  buffer[9] = 0x7f;
  buffer[10] = majVer;
  return ZMQ_PGREET_LEN;
}

// remainder of 64 byte greeting
uint8_t ZMQSocket::r_greeting(){
  uint8_t len = ZMQ_RGREET_LEN;
  memset(buffer, 0, len); // most bits are 0
  buffer[0] = minVer;
  buffer[1] = 'N';
  buffer[2] = 'U';
  buffer[3] = 'L';
  buffer[4] = 'L';
  return len;
}


uint8_t ZMQSocket::handshake_REQ(){
  uint8_t len = ZMQ_HS_REQ_LEN;
  for (uint8_t k = 0; k < len; k++){
    buffer[k] = pgm_read_byte_near(REQHandshake + k);
  }
  return len;
}

uint8_t ZMQSocket::handshake_PUSH(){
  uint8_t len = ZMQ_HS_PUSH_LEN;
  for (uint8_t k = 0; k < len; k++){
    buffer[k] = pgm_read_byte_near(PUSHHandshake + k);
  }
  return len;
}

// handshakes
uint8_t ZMQSocket::handshake(){
  uint8_t len = 0;
  switch(socketType){
    case REQ: 
      len = handshake_REQ();
      break;
    case PUSH:
      len = handshake_PUSH();
      break;
    default:
      return len;
  }
  return len; // 0 len message
}

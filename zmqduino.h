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

#ifndef ZMQDUINO
#define ZMQDUINO

#include "UIPEthernet.h"
#include "UIPClient.h"

#define ZMQ_MAX_LENGTH 64 // greeting is 64 bytes

#define ZMQ_RESP_TIMEOUT_MS 1000 

const uint8_t zmq_maj_ver = 3;
const uint8_t zmq_min_ver = 0;

enum zmq_soc { 
  REQ, // Request
  PUSH // Push
};

// this char array will hold all the ZMQ messages, so make it the maximum length
char zmq_buffer[ZMQ_MAX_LENGTH] = {0};

// zmq negotiate connection
uint8_t zmq_connect( EthernetClient& client, IPAddress host, uint16_t port, zmq_soc soc );

// perform greeting ritual
int8_t zmq_greet( EthernetClient& client );

// perform handshake ritual
int8_t zmq_send_handshake( EthernetClient& client, zmq_soc soc );

// send data to server (data is stored in zmq_buffer)
void zmq_sendData( EthernetClient& client, uint8_t len );

uint8_t zmq_receiveData( EthernetClient& client, uint8_t setup );

// negative -> error
// positive -> new bytes in zmq_buffer
int16_t zmq_readData( EthernetClient& client, uint8_t setup );

// send data to server
void sendData( EthernetClient& client, const char* msg, uint8_t len );

uint8_t waitForServer(uint16_t timeout);

#endif

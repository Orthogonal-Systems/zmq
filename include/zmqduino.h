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
#define ZMQ_MSG_OFFSET 4  // 4 byte header for ZMQ messages

#define ZMQ_RESP_TIMEOUT_MS 10000 

enum ZMQSocketType { 
  REQ, // Request
  PUSH // Push
};

class ZMQSocket {
  public:
    ZMQSocket( EthernetClient& _client, char* _buf, ZMQSocketType _socketType)
      : client(_client)
      , buffer(_buf)
      , socketType(_socketType)
    {}

    // zmq negotiate connection
    int8_t connect( IPAddress host, uint16_t port );

    // send data to server (msg is already formatted and stored in zmq_buffer)
    void send( uint8_t len );

    // send data to server, no formatting done
    void send( const char* msg, uint8_t len );

    // format header and send a ZMQ message that is already stored in the buffer
    void sendZMQMsg( uint8_t msgLen );
    
    int16_t read();
    int16_t recv();

    uint8_t waitForServer();

  private:
    static const uint8_t majVer = 3;
    static const uint8_t minVer = 0;
    EthernetClient client;  //!< Arduino client class
    char* const buffer;           //!< ZMQ communication buffer
    const ZMQSocketType socketType;
    const uint16_t timeout = ZMQ_RESP_TIMEOUT_MS;

    // perform greeting ritual
    int8_t greet();
    // perform handshake ritual
    int8_t sendHandshake();

    // negative -> error
    // positive -> new bytes in zmq_buffer
    //int16_t zmq_readData( EthernetClient& client );
    int16_t recv( uint8_t setup );

    int16_t read( uint8_t setup );
    uint8_t p_greeting();
    uint8_t r_greeting();
    uint8_t handshake();
    uint8_t handshake_REQ();
    uint8_t handshake_PUSH();
};

#endif

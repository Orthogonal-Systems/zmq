#ifndef ORIGIN
#define ORIGIN

#include <stdint.h>
#include <UIPEthernet.h>
#include <UIPClient.h>
#include "datapacket.h"
#include "zmqduino.h"

#define STREAM_NAME_MAX_LENGTH 10

#define ERR_CONNECTION  1
#define ERR_NODATA      2

class Origin{
  public:
    Origin( EthernetClient& _client
          , IPAddress _server_ip
          , uint16_t _reg_port
          , uint16_t _msg_port
          , char* _stream_name
          , uint8_t _stream_name_length
          , uint8_t _fracSecTS
          , uint8_t _max_channels
          , uint8_t _channels
          , char* _dtype_str
          , uint8_t _dtype_size
          , char* _zmq_buffer
        ) 
      : client(_client)
      , ZMQPush(_client, _zmq_buffer, PUSH)
      , packet(_channels, _stream_name, _stream_name_length, _dtype_size, _zmq_buffer+4, _fracSecTS)
      , ZMQbuffer(_zmq_buffer) 
      , MSGbuffer(_zmq_buffer+4) 
    {
      server_ip = _server_ip;
      reg_port = _reg_port;
      msg_port = _msg_port;
      memcpy(dtype_str,_dtype_str,DTYPE_STRING_MAXSIZE);
    }

    //! register the data stream with the server
    uint8_t registerStream();

    //! open the connection to the data server before beginning the data stream
    uint8_t setupDataStream();

    //! put the readings into the data packet according to the format used
    inline uint8_t preparePacket( uint32_t ts_sec, uint32_t ts_fsec, int16_t* readings ){
      return packet.preparePacket( ts_sec, ts_fsec, readings );
    }

    //! send the zmq packet already loaded into the buffer
    inline void sendPacket(uint8_t len){
      ZMQPush.sendZMQMsg(len);
    }

    //! put the readings into the data packet and then immediately send the packet
    uint8_t sendPacket( uint32_t ts_sec, uint32_t ts_fsec, int16_t* readings );

  private:
    EthernetClient client;    //! ethernet client class
    IPAddress server_ip;      //! origin server ip address
    ZMQSocket ZMQPush;        //! push-pull socket
    uint16_t reg_port;        //! server stream registration port
    uint16_t msg_port;        //! server data stream port

    DataPacket packet;        //! data packet class
    char * dtype_str[DTYPE_STRING_MAXSIZE];
    char * const ZMQbuffer;   //! pointer to start of buffer
    char * const MSGbuffer;   //! pointer to start of buffer after ZMQ header
};

#endif

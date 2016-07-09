#ifndef DATAPACKET
#define DATAPACKET

#include <stdint.h>
#include <string.h>

#define CHANNEL_NAME_SIZE 3 // len('cxx') = 3
#define DTYPE_STRING_MAXSIZE 6 // len('uint16')=6
#define INT8_TYPE_SIZE 1
#define INT16_TYPE_SIZE 2
#define INT32_TYPE_SIZE 4
#define INT64_TYPE_SIZE 8
#define FLOAT_TYPE_SIZE 4
#define TIMESTAMP_SIZE INT64_TYPE_SIZE   // 64b = 8B timestamp
#define STREAM_KEY_LENGTH 4 

// accepted types for server
// all type designations needs to be length = DTYPE_STRING_MAXSIZE
#define DTYPE_INT8    "int8  "
#define DTYPE_UINT8   "uint8 "
#define DTYPE_INT16   "int16 "
#define DTYPE_UINT16  "uint16"
#define DTYPE_INT32   "int32 "
#define DTYPE_UINT32  "uint32"
#define DTYPE_INT64   "int64 "
#define DTYPE_UINT64  "uint64"
#define DTYPE_float   "float "

#define ERR_SERVER_RESP 1
#define ERR_SERVER_LEN  2

class DataPacket{
  public:
    // copy stream name into class
    DataPacket( uint8_t _channels
        , char* _streamName
        , uint8_t _nameLength
        , uint8_t _entrySize
        , char* _buffer
        , uint8_t _fracSecTS
        )
      : channels(_channels)
      , streamName(_streamName)
      , streamNameSize(_nameLength)
      , dataEntrySize(_entrySize)
      , buffer(_buffer)
      {
        // TODO: find better way
        // calculate regstration packet size
        registerSize = streamNameSize + channels*( 2 + CHANNEL_NAME_SIZE + DTYPE_STRING_MAXSIZE );
        // calculate data packet size
        packetSize = INT32_TYPE_SIZE + TIMESTAMP_SIZE + channels*( dataEntrySize );
        fracSecTS = _fracSecTS;
      }

    // ready special packet in the buffer to send to sever to announce the data format
    // then call send function to send it
    // dtype is the data type string, use defined values above
    // dtype is the monitoring server data type designation:
    // Ex: [int8,uint8,int16,uint16,, ... , float]
    // returns length of packet in bytes
    uint8_t registerStream(char* dtype);
    // read the message in the buffer of length len
    // extract streamKey and return 0 if success, else return error code
    uint8_t processStreamRegistration(uint8_t len);
    // ready the data packet in the buffer
    // returns length of packet in bytes
    uint8_t preparePacket( uint32_t timestamp, int16_t* data );
    uint8_t preparePacket( uint32_t timestamp, uint32_t fsTS, int16_t* data );
    uint8_t preparePacket( uint32_t timestamp, float* data );
    uint8_t preparePacket( uint32_t timestamp, uint32_t fsTS, float* data );

    uint8_t getRegisterSize(){
      return registerSize;
    }
    uint8_t getPacketSize(){
      return packetSize;
    }


  private:
    const uint8_t channels; //! number of data points to send
    char * const streamName;  //! human readable stream designator
    char streamKey[STREAM_KEY_LENGTH];  //! binary stream designator, received from server after registration
    const uint8_t streamNameSize; //! length of stream name
    const uint8_t dataEntrySize;  //! size of each entry in bytes
    char * const buffer;  //! pointer to start of buffer (4 bytes after start of zmq buffer)

    // packet size
    uint8_t fracSecTS;
    uint8_t registerSize;   // regsiter stream packet size in bytes
    uint8_t packetSize;     // data packet size in bytes

    // convert from data type to char array with network byte order, and write into the buffer
    // pass pointer to the location in the buffer where data starts and fill it
    static uint8_t int2charArray(int16_t x, char* buf){
      for(uint8_t i=0; i<INT16_TYPE_SIZE; i++){
        buf[i] = char(x >> ((INT16_TYPE_SIZE-i-1)*8));
      }
      return INT16_TYPE_SIZE;
    }
    static uint8_t int2charArray(uint32_t x, char* buf){
      for(uint8_t i=0; i<INT32_TYPE_SIZE; i++){
        buf[i] = char(x >> ((INT32_TYPE_SIZE-i-1)*8));
      }
      return INT32_TYPE_SIZE;
    }
    // implementation from stack exchange # 24420246, user: Patrick Collins
    static uint8_t float2charArray(float x, char* buf){
      uint32_t asInt = *((uint32_t*)&x);
      return int2charArray(asInt, buf);
    }

    uint8_t addChannelStr( uint8_t ch, char* buf );
    uint8_t addPreString();

    // add the channel register sequence to the buffer
    // pass pointer to the location in the buffer where data starts and fill it
    // dtype is the data type string, use defined values above
    void registerChannel( uint8_t ch, char* buf, char* dtype );

    void addChannelData( uint8_t ch, int16_t x, char* buf );
    void addChannelData( uint8_t ch, float x, char* buf );

    // The stream identification key length 4, recieved from server after string registration
    inline void streamRegistrationKey(char* key){
      memcpy(streamKey,key,STREAM_KEY_LENGTH);
    }
};

#endif

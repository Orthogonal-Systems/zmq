#include <stdint.h>

#define CHANNEL_NAME_SIZE 2 // len('xx') = 2
#define INT_TYPE_SIZE 3     // len('int') = 3
#define TIMESTAMP_SIZE 10   // 2^32 = 4E10

class DataPacket{
  public:
    // copy stream name into class
    DataPacket( uint8_t _channels
        , char* _streamName
        , uint8_t _nameLength
        , uint8_t _entrySize
        , char* _buffer
        )
      : channels(_channels)
      , streamName(_streamName)
      , streamNameSize(_nameLength)
      , dataEntrySize(_entrySize)
      , buffer(_buffer)
      {
        // calculate regstration packet size
        registerSize = 6 + streamNameSize + channels*( 6 + CHANNEL_NAME_SIZE + INT_TYPE_SIZE );
        // calculate data packet size
        packetSize = 7 + streamNameSize + TIMESTAMP_SIZE + channels*( 4 + CHANNEL_NAME_SIZE + dataEntrySize );
      }

    // ready special packet in the buffer to send to sever to announce the data format
    // then call send function to send it
    // returns length of packet in bytes
    uint8_t registerStream();
    // ready the data packet in the buffer
    // returns length of packet in bytes
    uint8_t preparePacket( uint32_t timestamp, int16_t* data );

  private:
    // packet size
    uint8_t registerSize;   // regsiter stream packet size in bytes
    uint8_t packetSize;     // data packet size in bytes
    const uint8_t dataEntrySize;  // size of each entry (max int to decimal places)
    const uint8_t channels; // number of data points to send

    const uint8_t streamNameSize;     // length of stream name
    char * const streamName;  // stream designator with extra common json stuff

    char * const buffer;  // pointer to start of buffer (4 bytes after start of zmq buffer)

    // convert from int16 to char array with space padding
    // pass pointer to the location in the buffer where data starts and fill it
    void int2charArray(int16_t x, char* data);
    void int2charArray(uint32_t x, char* data);

    uint8_t addChannelStr( uint8_t ch, char* buf );
    uint8_t addPreString();

    // add the channel register sequence to the buffer
    // pass pointer to the location in the buffer where data starts and fill it
    void registerChannel( uint8_t ch, char* buf );

    void addChannelData( uint8_t ch, int16_t x, char* buf );
};

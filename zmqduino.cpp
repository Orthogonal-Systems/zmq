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

#include "Arduino.h"
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

const String fail = "Error: ";
const String conn = "Connected";
const String handshake = "Handshook";

char zmq_buffer[ZMQ_MAX_LENGTH] = {0};

// zmq negotiate connection
uint8_t zmq_connect( EthernetClient& client, IPAddress host, uint16_t port, zmq_soc soc ){
  int8_t err = 1;
  uint8_t success = 0;  
  uint8_t ret = 0;  
  do {
    if( err < 0 ){
      Serial.print(fail);
      Serial.println(err);
      delay(1000);
    }

    err = client.connect(host, port);
    // client. connect returns negative error number, success is 1
    if( err > 0 ){
      Serial.println(conn);
  
      // send greeting to server, and check stuff
      ret = zmq_greet(client);
      if( ret ){
        Serial.print(fail);
        Serial.println(ret);
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
  ret = zmq_send_handshake(client, soc);
  if( ret < 0 ){
    Serial.print(fail);
    Serial.println(ret);
    return ret;
  }
  Serial.println(handshake);
  return 0;
}

int8_t zmq_greet( EthernetClient& client ){
  // send partial greeting
  uint8_t len = zmq_p_greeting();
  zmq_sendData( client, len );

  // recieve partial greeting, into zmq_buffer
  len = zmq_receiveData( client, 1 );
  if( len < 0 ){
    return len;
  }
  // verify major version (remember we are actually only getting 10 bytes from python code)
  uint8_t alternate_greeting = 0;
  switch(len){
    case 10:
      alternate_greeting = 1;
      break;
    case 11:
      if( zmq_buffer[11] != zmq_maj_ver ){
        return 1;
      }
      break;
    default:
      return 2;
  }

  // send remaining greeting
  len = zmq_r_greeting();
  zmq_sendData( client, len );

  // recieve remaining greeting
  len = zmq_receiveData( client, 1 );
  if( len < 0 ){
    return len;
  }

  uint16_t shitty_chksum = 0;
  for( uint8_t i=1; i<5; i++ ){
    shitty_chksum += zmq_buffer[i+alternate_greeting];
  }
  // checking null mechanism
  if( shitty_chksum != 'N'+'U'+'L'+'L' ){
    return 3;
  }
  // checking rest of null mechanism
  for( uint8_t i=6; i<20; i++ ){
    if( zmq_buffer[i+alternate_greeting] ){
      return 3;
    }
  }
  // we good
  return 0;
}

int8_t zmq_send_handshake( EthernetClient& client, zmq_soc soc ){
  // send handshake
  uint8_t len = zmq_handshake(soc);
  zmq_sendData( client, len );
  // recieve handshake
  uint16_t resp_len = zmq_receiveData( client, 1 );
  if( resp_len < 0 ){
    return resp_len;
  }
  // TODO: verify parameters
  return resp_len;
}

// send data to server (data is stored in zmq_buffer)
void zmq_sendData( EthernetClient& client, uint8_t len ){
  Serial.print("\nSending bytes: ");
  Serial.println(len);
  client.write((uint8_t*)zmq_buffer, len);
}

int16_t zmq_receiveData( EthernetClient& client, uint8_t setup ){
  if( waitForServer(client, ZMQ_RESP_TIMEOUT_MS) ){
    Serial.println("timeout");
    return -1;
  }
  return zmq_readData( client, setup );
}
/*
uint16_t zmq_readData( EthernetClient& client ){
  return zmq_readData( client, 0 );
}
*/
// negative -> error
// positive -> new bytes in zmq_buffer
int16_t zmq_readData( EthernetClient& client, uint8_t setup ){
  uint8_t len = client.available();
  Serial.print("\nRecv bytes: ");
  Serial.println(len);
  if( setup ){
    client.read((uint8_t*)zmq_buffer, len);
    return len;
  }
  if( len < 4 ){
    client.read((uint8_t*)zmq_buffer, len);// clear buffer
    return -2;
  }
  client.read((uint8_t*)zmq_buffer, 4);
  // check that it isnt a long message, if it is clear it?
  if( zmq_buffer[0] & (1<<ZMQ_LONG_FLAG) ){
    return -3;
  }
  // check that there are at least the number of bytes we expect
  if( len - 4 < zmq_buffer[3] ){
    return -4;
  }
  // read the bytes sepecifed into the buffer
  len = zmq_buffer[3];
  client.read((uint8_t*)zmq_buffer, len);
  return len;
}

// send data to server
void sendData( EthernetClient& client, const char* msg, uint8_t len ){
  Serial.print("\nSending bytes: ");
  Serial.println(len);
  client.write((uint8_t*)msg, len);
}

uint8_t waitForServer(EthernetClient& client, uint16_t timeout){
  // wait for response from server
  uint16_t i = 0;
  while( !client.available() ){
    if( i > timeout ){
      return 1;
    }
    i++;
    delay(10);
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
uint8_t zmq_p_greeting(){
  zmq_buffer[0] = 0xff;
  // bits 1-8 are "dont care"
  zmq_buffer[9] = 0x7f;
  zmq_buffer[10] = zmq_maj_ver;
  return ZMQ_PGREET_LEN;
}

// remainder of 64 byte greeting
uint8_t zmq_r_greeting(){
  uint8_t len = ZMQ_RGREET_LEN;
  memset(zmq_buffer, 0, len); // most bits are 0
  zmq_buffer[0] = zmq_min_ver;
  zmq_buffer[1] = 'N';
  zmq_buffer[2] = 'U';
  zmq_buffer[3] = 'L';
  zmq_buffer[4] = 'L';
  return len;
}


uint8_t zmq_handshake_REQ(){
  uint8_t len = 40;
  zmq_buffer[23] = 3; // 4 byte property value length
  zmq_buffer[24] = 'R';
  zmq_buffer[25] = 'E';
  zmq_buffer[26] = 'Q';
  // property
  zmq_buffer[27] = 8;
  zmq_buffer[28] = 'I';
  zmq_buffer[29] = 'd';
  zmq_buffer[30] = 'e';
  zmq_buffer[31] = 'n';
  zmq_buffer[32] = 't';
  zmq_buffer[33] = 'i';
  zmq_buffer[34] = 't';
  zmq_buffer[35] = 'y';
  for(uint8_t i=0; i<4; i++){
    zmq_buffer[len-i] = 0;
  }
  return len;
}

uint8_t zmq_handshake_PUSH(){
  uint8_t len = ZMQ_HS_PUSH_LEN;
  zmq_buffer[23] = 4; // 4 byte property value length
  zmq_buffer[24] = 'P';
  zmq_buffer[25] = 'U';
  zmq_buffer[26] = 'S';
  zmq_buffer[27] = 'H';
  return len;
}

// handshakes
uint8_t zmq_handshake( zmq_soc soc ){
  uint8_t len = 0;
  switch(soc){
    case REQ: 
      len = zmq_handshake_REQ();
      break;
    case PUSH:
      len = zmq_handshake_PUSH();
      break;
    default:
      return len;
  }
  zmq_buffer[0] = 1<<ZMQ_CMD_FLAG; // command flag
  zmq_buffer[1] = len - 2;  // message length minus header
  // command
  zmq_buffer[2] = 5;  // length of command
  zmq_buffer[3] = 'R';
  zmq_buffer[4] = 'E';
  zmq_buffer[5] = 'A';
  zmq_buffer[6] = 'D';
  zmq_buffer[7] = 'Y';
  // property
  zmq_buffer[8] = 11;
  zmq_buffer[9] = 'S';
  zmq_buffer[10] = 'o';
  zmq_buffer[11] = 'c';
  zmq_buffer[12] = 'k';
  zmq_buffer[13] = 'e';
  zmq_buffer[14] = 't';
  zmq_buffer[15] = '-';
  zmq_buffer[16] = 'T';
  zmq_buffer[17] = 'y';
  zmq_buffer[18] = 'p';
  zmq_buffer[19] = 'e';
  zmq_buffer[20] = 0;
  zmq_buffer[21] = 0;
  zmq_buffer[22] = 0;
  return len; // 0 len message
}

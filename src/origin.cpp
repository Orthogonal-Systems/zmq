#include <stdint.h>
//#include <UIPEthernet.h>
//#include <UIPClient.h>
#include "datapacket.h"
//#include "zmqduino.h"
#include "origin.h"


uint8_t Origin::registerStream(){
  ZMQSocket ZMQReq( client, ZMQbuffer, REQ );
  int16_t len;
  uint8_t err;
  do{
    err = 0;
    len = -1;
    if( ZMQReq.connect( server_ip, reg_port ) ){
      ZMQReq.stop();
      err = ERR_CONNECTION;
    }
    if(!err){
      // register datastream with server
      len = packet.registerStream(*dtype_str);
      ZMQReq.sendZMQMsg(len);
      len = ZMQReq.recv();
      if( len < 0 ){
        ZMQReq.stop();
        err = ERR_NODATA;
      }
    }
  } while( len < 0 );
  err = packet.processStreamRegistration(len);
  ZMQReq.stop();// disconnect from registering port
  return err;
}

uint8_t Origin::setupDataStream(){
  if( ZMQPush.connect( server_ip, msg_port ) ){
    ZMQPush.stop();
    return ERR_CONNECTION;
  }
  return 0;
}

uint8_t Origin::sendPacket( uint32_t ts_sec, uint32_t ts_fsec, int16_t* readings ){
  uint8_t len = preparePacket(ts_sec, ts_fsec, readings);
  ZMQPush.sendZMQMsg( len );
  return len;
}

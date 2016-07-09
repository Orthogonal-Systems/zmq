#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "datapacket.h"

using std::cout;
using std::endl;

int main(){
  cout << "commencing unit tests for data packet compliance\n";
  char buffer[256] = {0};
  int channels = 2;
  // try out the registration method
  DataPacket packet(channels,(char *)"ABC",3,INT16_TYPE_SIZE,buffer,1);
  packet.registerStream((char*)DTYPE_INT16);
  cout << "data in buffer (bytes): " << int(packet.getRegisterSize()) << endl;
  for(uint8_t j=0; j<packet.getRegisterSize(); j++){
    cout << buffer[j];
  }
  cout << endl;

  // try out the data packet method
  uint32_t time = 1468032801;
  uint32_t frac_sec = 0x1ac0b3b0;
  int16_t data[channels] = {0xef34,0x2312};
  char regResp[9] = "[0,\xAA\xBB\xCC\xDD]"; // generate success response with fake streamKey
  memcpy(buffer, regResp, 8);
  uint8_t err = packet.processStreamRegistration(8);      // process the fake response
  cout << "Registration processing error code: " << int(err) << endl;
  packet.preparePacket(time, frac_sec, data); // send out a data packet
  cout << "data in buffer (bytes): " << int(packet.getPacketSize()) << endl;
  for(uint8_t j=0; j<packet.getPacketSize(); j++){
    printf("%02X:", buffer[j]&0x000000FF);
  }
  cout << endl;

  // try a error in the response
  memcpy(buffer, "[1,\xAA\xBB\xCC\xDD]", 8);
  err = packet.processStreamRegistration(9);      // process the fake response
  cout << "Registration processing error code: " << int(err) << endl;
}

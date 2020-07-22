/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Main ESPNow Stuff can be found here!
*/
#include "MasterESPnow.h"

bool ESPNowMaster::check_n_pair(const uint8_t *mac_addr, const uint8_t *data, int data_len){
 char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  
  #ifdef DEBUG
  printf("Last Packet Recv from: ");
  printf(macStr); 
  #endif
  
  if(data_len == 4){    
    if(data[0] == 20 && data[1] == 30 && data[2] == 40 && data[3] == 40){
      
      #ifdef DEBUG
      printf("Let's pair with this device!");
      #endif
      
      // used for adding slave
      uint8_t mac[6] = {mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]};
      uint8_t *point_mac = mac; 

      // used for broadcasting new mac address to network
      int int_mac[6] = {mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]};
      this->add_slave(int_mac, ESP_CHANNEL);
      this->pair_slaves();
      this->send_data(this->slave_count-1, point_mac, 6);
      return true; 
    }  
  }
  return false;
}

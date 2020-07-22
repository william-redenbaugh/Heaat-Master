/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Pairing stuff can be found here!
*/
#include "MasterESPnow.h"

bool ESPNowMaster::check_slave_paired(const uint8_t *mac_add){
  return esp_now_is_peer_exist(mac_add);
}

int ESPNowMaster::device_by_id(const uint8_t *mac_add){
  if(this->check_slave_paired(mac_add)){
    // runs through list of devices
    for(int i = 0; i <= this->slave_count; i++){
      int x = 0; 
      // then checks the last 6 mac address(or 3 bytes) digits, since the first three are defined by manufacture
      for(int j = 2; j < 6; j++){
        if(this->slaves[i].peer_addr[j] == mac_add[i]){
          x++; 
        }
      }
      // if there is a complete match of the last 3 bytes
      if(x == 3){
          // return which device in the list
          return i;
        }
    }
  }
  return -1;
}

// adds a slave from the slave array!
void ESPNowMaster::add_slave(int *mac_add, int channel){
  
  // if there is a current slave already in array
  // checks to see if we are adding a mac address already added
  // otherwise we don't add it
  if(slave_count > 1){
    for(int i = 0; i < this->slave_count; i++){
      int g = 0; 
      for(int m = 0; m < 6; m++){
        if(this->slaves[i].peer_addr[m] == mac_add[m]){
          g++;  
        }
      }
      if(g == 6){
        #ifdef DEBUG
        printf("Device already added to slave list!, cancelling add");
        #endif
        return; 
      }
    }
  }

  if(slave_count > 19){
    #ifdef DEBUG
    printf("No more device spots fit into espnow buffer! Not adding device");  
    #endif
  }
  // reads mac addres into slave array
  for(int i = 0; i < 6; i++){
    this->slaves[this->slave_count].peer_addr[i] = mac_add[i]; 
  }
  // set channel for a specific slave
  this->slaves[slave_count].channel = channel;
  // no encryption for now
  this->slaves[this->slave_count].encrypt = 0;
  // next slave counted!
  this->slave_count++;
}

// resets/ deletes all slave
void ESPNowMaster::reset_slaves(){
  slave_count = 0; 
  memset(this->slaves, 0, sizeof(this->slaves));
  err = esp_now_deinit();
  if(err == ESP_OK){
    #ifdef DEBUF
    printf("ESP cleared slaves!");  
    #endif
  }
  else{
    #ifdef DEBUG
    printf("ESP couldn't delete all the slaves :(");  
    #endif
  }

  err = esp_now_init();
  
  if(err == ESP_OK){
    #ifdef DEBUF
    printf("ESP-Now restarted properly");  
    #endif
  }
  else{
    #ifdef DEBUG
    printf("ESP-now couldn't restart properly");  
    #endif
  }
}

// pair all slaves! from slave array!
void ESPNowMaster::pair_slaves(){
  if(slave_count == 0){
    #ifdef DEBUG
    printf("no slaves to pair... please add some!");
    #endif    
  }
  else{
    // runs through slave, and sets a peer pointer to slave
    for(int i = 0; i < this->slave_count; i++){
      esp_now_peer_info_t *peer = &this->slaves[i];
      uint8_t* peer_addr = this->slaves[i].peer_addr;
      // if check if peer exists
      bool exist = esp_now_is_peer_exist(peer_addr);
      
      #ifdef DEBUG
      // prints out the slaves ID by mac address
      printf("Slave ID: \n");
      for (int ii = 0; ii < 6; ++ii ) {
        printf("%d ",slaves[i].peer_addr[ii]);
        if (ii != 5) printf(":");
      }
      printf(" Status: ");
      #endif

      // if device is paired, we let the debugger know!
      if(exist){
        #ifdef DEBUG
        printf("Device Already Paired!\n");
        #endif  
      }
      // if device aint parieds
      else {
          // Slave not paired, attempt pair
          esp_err_t addStatus = esp_now_add_peer(peer);
          if (addStatus == ESP_OK) {
            // Pair success
            #ifdef DEBUG
            printf("Pair success\n");
            #endif  
          } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
            // How did we get so far!!
            #ifdef DEBUG
            printf("ESPNOW Not Init\n");
            #endif  
          } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
            #ifdef DEBUG
            printf("Add Peer - Invalid Argument\n");
            #endif  
          } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
            #ifdef DEBUG
            printf("Peer list full\n");
            #endif  
          } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
            #ifdef DEBUG
            printf("Out of memory\n");
            #endif  
          } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
            #ifdef DEBUG
            printf("Peer Exists\n");
            #endif  
          } else {
            #ifdef DEBUG
            printf("Not sure what happened\n");
            #endif  
          }
          // wait 100ms per device. 
          vTaskDelay(TickType_t(100 / portTICK_PERIOD_MS));  
        }
    }
  }
}

// sends data to defined slave 
void ESPNowMaster::send_data(uint8_t slave_count, uint8_t* data, int size){
  if(this->slave_count < slave_count){
    #ifdef DEBUG
    printf("Don't ask for a slave you can't reach!");
    #endif  
  }
  
  else{
    // sets peer address
    const uint8_t *peer_addr = slaves[slave_count].peer_addr; 
    // sends data array of defined size(because pointers)
    err = esp_now_send(peer_addr, data, size);
    // if it worked fined
    if (err == ESP_OK) {
      #ifdef DEBUG
      printf("Data was sent successfully\n");
      #endif 
    }

    // lets us know what went wrong
    #ifdef DEBUG
      else if (err == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      printf("ESPNOW not Init.\n");
      } else if (err == ESP_ERR_ESPNOW_ARG) {
      printf("Invalid Argument\n");
      } else if (err == ESP_ERR_ESPNOW_INTERNAL) {
      printf("Internal Error\n");
      } else if (err == ESP_ERR_ESPNOW_NO_MEM) {
      printf("ESP_ERR_ESPNOW_NO_MEM\n");
      } else if (err == ESP_ERR_ESPNOW_NOT_FOUND) {
      printf("Peer not found.\n");
      } else {
      printf("Not sure what happened\n");
    }
    #endif
  } 
}

// if broadcasting is setup, then send data of a particular size!
void ESPNowMaster::broadcast(uint8_t* data, int size){
  // sends data should broadcast setup 
  if(broadcast_setup){
    this->send_data(0, data, size); 
  }
}

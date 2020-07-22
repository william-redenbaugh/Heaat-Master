/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Call espnow stuff can be found here!
*/

#include "MasterESPnow.h"

// deregisters sending callback!
void ESPNowMaster::disable_send_callback(){
  err = esp_now_unregister_send_cb();
  if(err == !ESP_OK){
    #ifdef DEBUG
    printf("ESP couldn't deregister callback!");
    #endif
  }
}

void ESPNowMaster::disable_receive_callback(){
  // deregiesters callback!
  err = esp_now_unregister_recv_cb();
  if(err == !ESP_OK){
    #ifdef DEBUG
    printf("ESP couldn't deregister callback!");
    #endif
  }
}

void ESPNowMaster::attach_receive_callback(esp_now_recv_cb_t callback){
  // error checks adding espnow callback!
  err = esp_now_register_recv_cb(callback);
  if(err == !ESP_OK){
    #ifdef DEBUG
    printf("ESP couldn't attach a recieve callback, please check to see if ESPNOW is initialized");
    #endif
  }
}

// adds callback on receive!
void ESPNowMaster::attach_send_callback(esp_now_send_cb_t callback){
  esp_now_register_send_cb(callback);
}
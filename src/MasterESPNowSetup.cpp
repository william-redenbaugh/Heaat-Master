/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Other espnow setup stuff can be found here!
*/
#include "MasterESPnow.h"
#include <stdio.h>

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // just the default callback that's set when you start
  // if debug mode is enabled, the  we read all the data sent from the packet
  #ifdef DEBUG
  printf("Last Packet Sent to: %02x:%02x:%02x:%02x:%02x:%02x ", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  printf(" Last Packet Send Status: ");
  printf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success \n" : "Delivery Fail \n");
  #endif
}

void ESPNowMaster::begin(){
  // start wifi first and foremost
  //printf("starting espnow!");
  this->start_wifi();
  // try starting espnow a couple times!
  uint8_t count = 0; 
  while(1){
    this->err = esp_now_init(); 
    if(this->err == ESP_OK){
      #ifdef DEBUG
      printf("ESP Initialized properly!\n");
      #endif  
      break;
    }
    #ifdef DEBUG
    printf("Hmm.. something went wrong :( Retrying...\n");
    #endif  
      
    vTaskDelay(TickType_t(100 / portTICK_PERIOD_MS));  
    count++; 
    if(count == 11){
      // if espnow couldn't startup, we restart, just hopefully that will fix the problem
      #ifdef DEBUG
      printf("ESP Couldn't Initialize.. restarting!\n");
      #endif  
      esp_restart();  
    }  
  }
  // attaches the default callback
  this->attach_send_callback(OnDataSent);
}

void ESPNowMaster::start_wifi(){
   // starts tcp over ip adapter
  tcpip_adapter_init();
  // creates event loop
  //ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK( esp_wifi_start());

  /* In order to simplify example, channel is set after WiFi started.
   * This is not necessary in real application if the two devices have
   * been already on the same channel.
   */
  ESP_ERROR_CHECK( esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_ABOVE) );

  // long range would be pretty cool!
  #if CONFIG_ENABLE_LONG_RANGE
  ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
  #endif
  esp_wifi_disconnect();
}

void ESPNowMaster::setup_broadcast(uint8_t channel){
  // broadcast ID is always 255(dec) for every macid qualifer
  int mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  this->add_slave(mac, channel);
  broadcast_setup = true; 
}
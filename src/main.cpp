/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Application and main code can be found here!
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "MasterESPnow.h"
#include "Serial.h"
#include "serial_handler_defines.h"

struct Main{  
  // serial interface!
  UartEsp* serial = new UartEsp(); 

  // espnow master control!
  ESPNowMaster* master = new ESPNowMaster();
};

Main sys; 

// literally only exists because IDF wasn't designed for C++ based apps :(
void pair_callback(const uint8_t *mac_addr, const uint8_t *data, int data_len){
  if(sys.master->check_n_pair(mac_addr, data, data_len)){
      // sends a packet letting computer know that device has paired!
      uint8_t arr[12] = {0};
      arr[0] = SERIAL_DEVICE_PAIRED;
      arr[1] = sys.master->slave_count;
      for(int i = 2; i < 8; i++){
        arr[i] = mac_addr[i-2];   
      }
      sys.serial->send_packet(arr);
  }
}

// whenever there is an animation completed
// send the mac address over!
void confirmation_callback(const uint8_t *mac_addr, const uint8_t *data, int data_len){
  // send a leading byte
  if(sys.master->check_slave_paired(mac_addr)){
    if(data[0] == 255){
      uint8_t arr[12] = {0};
      arr[0] = SERIAL_ANIMATION_COMPLETE;
      for(int i = 1; i < 7; i++){
        arr[i] = mac_addr[i-1];   
      }
      sys.serial->send_packet(arr);
    }
  }
}

void serial_handle_task(){
  for(;;){
    // waiting for incoming packet!
    sys.serial->wait_for_packet();
    // gets the earliest unhandled packet from serial bus(not necisarily latest packet!)
    std::vector<uint8_t> buff = sys.serial->get_packet();

    // so we can switch through all the different commands
    switch(buff[0]){
      
      // if the first packet is value 32
      // then it's just asking to change kelvin value of all the wireless devices. 
      case KELVIN_ALL:{
        // gets looks at the next two bytes(and ignores the other 9 bytes for now
        uint8_t data[2] = {buff[1], buff[2]};
        
        // send the data to all the devices
        for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, data, sizeof(data));
        }
        break;
      }
      
      // if first packet is 35
      // we send rgb values to all the wireless devices
      case RGB_ALL:{
        uint8_t data[3] = {buff[1], buff[2], buff[3]};
        
        for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, data, sizeof(data));
        }
        break;
      }
      
      // if first packet is 37, then we send kelvin value, and animation time 
      // to all devices
      case KELVIN_ANIMATION_ALL:{ 
        // moving data from serial buffer to devices
        uint8_t data[4] = {buff[1], buff[2], buff[3], buff[4]};
        
        for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, data, sizeof(data));
        }
        break;
      }
      
      // first packet is 39, then we send rgb value and animation time 
      // to all the devices
    case RGB_ANIMATION_ALL:{
        // moving data from serial buffer to devices
        uint8_t data[5] = {buff[1], buff[2], buff[3], buff[4], buff[5]};
        
        for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, data, sizeof(data));
        }
        break;
      }

     // sends kelvin to a specific device
    case KELVIN_DEVICE:{
      // reads kelvin bytes
      uint8_t data[2] = {buff[2], buff[3]};

      // 2nd byte tells us which device we want to send data to!
      sys.master->send_data(buff[1], data, sizeof(data));
      break;
     }

     // sends rgb to a specific device
    case RGB_DEVICE:{
      uint8_t data[3] = {buff[2], buff[3], buff[4]};
      sys.master->send_data(buff[1], data, sizeof(data));
      break;
     }

     // send kelvin and animation time 
     // to a specific device
    case KELVIN_TIME_DEVICE:{
      // moving data from serial buffer to devices
      uint8_t data[4] = {buff[2], buff[3], buff[4], buff[5]};
      sys.master->send_data(buff[1], data, sizeof(data));
      break;
     }

     // send RGB and animation time 
     // to a specific device
    case RGB_TIME_DEVICE:{
      // moving data from serial buffer to devices
      uint8_t data[5] = {buff[2], buff[3], buff[4], buff[5], buff[6]}; 
      sys.master->send_data(buff[1], data, sizeof(data));
      break;
     }

    case PAIRING_MODE:{
       sys.master->disable_receive_callback();
       sys.master->attach_receive_callback(pair_callback);
      // runs for about 15 seconds, then removes the receive callback, disabled pairing. 
      vTaskDelay(15000/ portTICK_PERIOD_MS);

      // gotta kill the callback to avoid pairing mode for too long!
      sys.master->disable_receive_callback();
      sys.master->attach_receive_callback(confirmation_callback);
      // currently here for debugging purposes
      uint8_t arr[12] = {120};
      sys.serial->send_packet(arr);
      break;
     }

     case RESTART_MODE:{
       uint8_t packet[1] = {69};
       for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, packet, sizeof(packet));
        }
        vTaskDelay(400 / portTICK_PERIOD_MS);
        esp_restart();
       break;
     }

     case BRIGHTNESS:{
       uint8_t packet[1] = {buff[1]};
       for(int i = 0; i < sys.master->slave_count; i++){
          sys.master->send_data(i, packet, sizeof(packet));
        }
        break;
     }
     
      case BRIGHTNESS_DEVICE:{
        uint8_t packet[1] = {buff[1]}; 
        sys.master->send_data(buff[2], packet, sizeof(packet));
      }
      break;

      case RGB_MAC:{
        uint8_t mac_buff[6] = {buff[1], buff[2], buff[3], buff[4], buff[5], buff[6]};
        uint8_t data[3] = {buff[7], buff[8], buff[9]};
        int x = sys.master->device_by_id(mac_buff);
        if(x >= 0){
            sys.master->send_data(x, data, 3);
        }
      }
      break;

      case KELVIN_MAC:{
        uint8_t mac_buff[6] = {buff[1], buff[2], buff[3], buff[4], buff[5], buff[6]};
        uint8_t data[2] = {buff[7], buff[8]};
        int x = sys.master->device_by_id(mac_buff);
        if(x >= 0){
            sys.master->send_data(x, data, 2);
        }
      }
      break;

      case RGB_TIME_MAC:{
        uint8_t mac_buff[6] = {buff[1], buff[2], buff[3], buff[4], buff[5], buff[6]};
        uint8_t data[5] = {buff[7], buff[8], buff[9], buff[10], buff[11]};
        int x = sys.master->device_by_id(mac_buff);
        if(x >= 0){
            sys.master->send_data(x, data, 5);
        }
      }
      break;

      case KELVIN_TIME_MAC:{
        uint8_t mac_buff[6] = {buff[1], buff[2], buff[3], buff[4], buff[5], buff[6]};
        uint8_t data[4] = {buff[7], buff[8], buff[9], buff[10]};
        int x = sys.master->device_by_id(mac_buff);
        if(x >= 0){
            sys.master->send_data(x, data, 4);
        }
      }
      break;
    }
    // wait a bit between requests!
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void setup() {

  sys.serial->begin(115200);
  // starts up the espnow stuff!
  sys.serial->write_string("device turned on!");
  printf("debugging works!");
  sys.master->begin();
  sys.serial->write_string("espnow enabled!");
  
  // waits for a packet!
  sys.serial->wait_for_packet();
  std::vector<uint8_t> buff = sys.serial->get_packet();
  
  // goes through all 12 bytes from packet and determines
  // if computer wants master node to enable pairing
  int tot = 0; 
  for(int i = 0; i < 12; i++){
    if(buff[i] == KELVIN_ALL){
      tot++; 
    }  
  }
  if(tot == PACKET_SIZE){
    sys.serial->write_string("device pairing!");
    // starts enables pairing mode callbacks so we can add devices
    sys.master->attach_receive_callback(pair_callback);
    // runs for about 15 seconds, then removes the receive callback, disabled pairing. 
    vTaskDelay(5000/ portTICK_PERIOD_MS);
    
    // gotta kill the callback to avoid pairing mode for too long!
    sys.master->disable_receive_callback();
    sys.master->attach_receive_callback(confirmation_callback);
    // currently here for debugging purposes
    sys.serial->write_string("Done being in pairing mode");
  }
}

esp_err_t setup_nvs_flash(){
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }
  return err; 
}

extern "C" void app_main(void){
  ESP_ERROR_CHECK(setup_nvs_flash());
  setup();
  for(;;){
    serial_handle_task();
  }
}
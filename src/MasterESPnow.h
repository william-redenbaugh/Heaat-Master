/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
All the ESPNow Master Stuff can be found here!
*/
#ifndef _MASTERESPNOW_H
#define _MASTERESPNOW_H

#include "esp_event.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <string.h>

// debug mode prints a bunch of stuff!
#define DEBUG

// maximum 20 devices connected to a single main device at a time!
#define NUMSLAVES 20
#define ESP_CHANNEL 3

// #define CONFIG_ENABLE_LONG_RANGE

class ESPNowMaster{
  public: 
    // begins wifi and esp now
    void begin(); 
    
    // adds a slave to the array
    void add_slave(int *mac_add, int channel);

    // removes and registers all slave
    void reset_slaves();

    // pairs all slaves, and should a slave be pared we chillin!
    void pair_slaves();

    // sends data to a defined slave in the array
    void send_data(uint8_t slave_count, uint8_t* data, int size);

    // add a sending callbac
    void attach_send_callback(esp_now_send_cb_t callback);

    // adding a receiving callback
    void attach_receive_callback(esp_now_recv_cb_t callback);
    
    // sets up broadcast if we wanna use that(but seriously dont :(
    void setup_broadcast(uint8_t channel);

    // disables recieving callback!
    void disable_receive_callback();

    // disables sending callbacks!
    void disable_send_callback();

    // should broadcast be defined, broadcast up
    void broadcast(uint8_t* data, int size);

    // returns device id by mac address!
    int device_by_id(const uint8_t *mac_add);

    // since the IDF was setup as a C based enviornment
    // I can't attach callbacks to methods 
    // instead I'll just call this function in the callback
    bool check_n_pair(const uint8_t *mac_addr, const uint8_t *data, int data_len);

    bool check_slave_paired(const uint8_t *mac_add);
    
    // total slave count!
    uint8_t slave_count = 0;
    
  private: 

    ESPNowMaster* point; 
    // starts wifi separatly
    void start_wifi();
    
    // error check
    esp_err_t err; 
    esp_now_peer_info_t slaves[NUMSLAVES] = {}; 

    EventGroupHandle_t data_wait;
    int wait_response = BIT0;

    bool broadcast_setup = false; 

};

#endif

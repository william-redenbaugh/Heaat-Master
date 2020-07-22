/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Serial head file with definitions can be found here!
*/
#ifndef _SERIAL_H
#define _SERIAL_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include <bits/stdc++.h>
 
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

using namespace std;

#define ECHO_TEST_TXD  (GPIO_NUM_1)
#define ECHO_TEST_RXD  (GPIO_NUM_3)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define PACKET_SIZE 12

#define BUF_SIZE (4096)

class UartEsp{
  public: 
    
    // uart configuation stuff! set this up before the begin command!
    uart_word_length_t data_bits = UART_DATA_8_BITS;
    uart_parity_t parity = UART_PARITY_DISABLE;
    uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
    uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE; 

    // specifed uart interface(there are three!)
    uart_port_t uart_interface = UART_NUM_1;

    esp_err_t error_check; 

    // these values are set by default, but you probably should change them before you call the 
    // begin command!
    int rx = ECHO_TEST_RXD; 
    int tx = ECHO_TEST_TXD; 
    int rts = ECHO_TEST_RTS;
    int cts = ECHO_TEST_CTS;

    // size of possible serial buffer!
    int buff_size = 1024; 
    uint8_t data_buff[1024]; 
    uint8_t delay_ammount = 5;
    
    // when you write a string, whether or not it will return a newln
    bool string_new_line = true; 
    uint16_t string_buff_len = 0;

    // begin command begins life for the serial interface. 
    void begin(int buad);
    
    // writes a string(converts to char array and sends up the serial bus)
    void write_string(string str); 
    
    // writes kinda like a string but with a char array, saves RAM
    void write(char char_array[]);

    // writes a single byte
    void write(uint8_t single_byte);

    // writes a byte array
    void write_array(uint8_t *byte_array, int size);

    // reads data1
    std::vector<uint8_t> get_data();

    int read_data();

    // tells the serial interface to run in the background!
    void read_background();

    // send packet_data
    void send_packet(uint8_t instruction_byte, uint8_t packet_size, uint8_t* data_arr);
    std::vector<uint8_t> get_packet();
    void process_packet();
    uint8_t wait_for_packet();
    void send_packet(uint8_t *packet);
    
    // packet variables
    std::vector<uint8_t> data_arr;
    std::vector<uint8_t> latest_packet;
    uint16_t current_data_amount = 0; 
    uint8_t packet_size = 0; 
 
    private:
      TaskHandle_t background_serial_handler;
      QueueHandle_t uart_queue;  
      QueueHandle_t packet_wait;  
      QueueHandle_t packet_send;  
      SemaphoreHandle_t packet_lock;
      uint8_t read_packet(uint8_t *data);
};

#endif

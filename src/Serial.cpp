/*
Author: William Redenbaugh
Last Edit Date: 6/6/19
Notes:
Main serial stuff can be found here!
*/
#include "Serial.h"

void read_serial_task(void *args){
  UartEsp *obj = static_cast<UartEsp*>(args); 
  obj->read_background();  
  vTaskDelete(NULL);
}

void process_packet_task(void *args){
  UartEsp *obj = static_cast<UartEsp*>(args); 
  obj->process_packet();
}

void UartEsp::begin(int buad){
  /* Configure parameters of an UART driver,
  * communication pins and install the driver */
  uart_config_t uart_config = {
      .baud_rate = buad,
      .data_bits = this->data_bits,
      .parity    = this->parity,
      .stop_bits = this->stop_bits,
      .flow_ctrl = this->flow_ctrl
  }; 
  ESP_ERROR_CHECK(uart_param_config(this->uart_interface, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(this->uart_interface, tx, rx, ECHO_TEST_RTS, ECHO_TEST_CTS));
  ESP_ERROR_CHECK(uart_driver_install(this->uart_interface, buff_size * 2, buff_size * 2, 20, &this->uart_queue, 0));
  xTaskCreate(read_serial_task, "Uart and packet read task", 4096, (void*)this, PACKET_SIZE, &background_serial_handler);
  xTaskCreate(process_packet_task, "Uart and packet read task", 4096, (void*)this, PACKET_SIZE, &background_serial_handler);
  // wait a bit to let the other tasks get ready!
  vTaskDelay(TickType_t(100 / portTICK_PERIOD_MS));  
  
  this->wait_for_packet();
  this->get_packet();
  
}

void UartEsp::write_string(string str){

  // creates a char array with values from the string
  if(string_new_line){
    string_buff_len = str.length() + 2;
    char char_array[string_buff_len]; 
    strcpy(char_array, (str + "\n").c_str());
    uart_write_bytes(this->uart_interface, char_array, string_buff_len);
  }
  else{
    string_buff_len = str.length() + 1;
    char char_array[string_buff_len]; 
    strcpy(char_array, str.c_str());
    uart_write_bytes(this->uart_interface, char_array, string_buff_len);
  }
}

void UartEsp::write(char char_array[]){
  uart_write_bytes(this->uart_interface, char_array, strlen(char_array));
}

void UartEsp::write(uint8_t single_byte){
  char buff[1] = {(char)single_byte};
  uart_write_bytes(this->uart_interface, buff, 1);
}

void UartEsp::write_array(uint8_t *byte_array, int size){
  string_buff_len = size;
  char *char_pointer = (char*)byte_array;
  uart_write_bytes(this->uart_interface, char_pointer, string_buff_len);
}


int UartEsp::read_data(){
  // read data and return length
  int len = uart_read_bytes(this->uart_interface, data_buff, buff_size, 10 / portTICK_RATE_MS);
  return len; 
}

std::vector<uint8_t> UartEsp::get_packet(){
  // copys packet over!
  std::vector<uint8_t> buff = this->latest_packet;
  latest_packet.clear();
  // we can release the packet lock! so now the next packet can be processed!
  xQueueSend(this->packet_send, &this->packet_size, portMAX_DELAY);
  return buff; 
}

uint8_t UartEsp::wait_for_packet(){
  uint8_t packet_size = 0; 
  if(xQueueReceive(this->packet_wait, &packet_size, portMAX_DELAY)){} 
  return packet_size;
}

void UartEsp::send_packet(uint8_t *packet){
    this->write_array(packet, PACKET_SIZE);
}

void UartEsp::read_background(){
  packet_lock = xSemaphoreCreateBinary();
  this->packet_wait = xQueueCreate(30, sizeof(uint8_t));
  
  if(packet_wait == NULL){
    this->write_string("error creating queue");  
  }

  packet_send = xQueueCreate(30, sizeof(uint8_t));

  if(packet_send == NULL){
    this->write_string("error creating queue");  
  }
  
  xQueueSend(this->packet_wait, &this->packet_size, portMAX_DELAY);
  
  uart_event_t event;
  latest_packet.reserve(4096);
  for(;;){
    // waiting on data to come in!
    if(xQueueReceive(this->uart_queue, (void*)&event, (portTickType)portMAX_DELAY)) {
      // if event has found new data
      switch(int(event.type)){
        // data case! note need to deal with other cases
        case UART_DATA:
          // calls read data_function
          this->read_data();
          for(int i = 0; i < event.size; i++){
            // takes data from buffer and puts in the data buffer!
            this->data_arr.push_back(data_buff[i]);
            // increments current data amount index!
            this->current_data_amount++;
          }
          break;
      }
    }
  }
}

void UartEsp::process_packet(){
  uint8_t queue_stuff = 0; 
  for(;;){
    // look!
    while(current_data_amount >= PACKET_SIZE){
      // waits and takes the packet_lock semaphore!
      if(xQueueReceive(this->packet_send, &queue_stuff, portMAX_DELAY)){
        // clears the latest packet
        latest_packet.clear();
        // sends the data over from the data array into the vector
        for(int i = 0; i < PACKET_SIZE; i++){
          latest_packet.push_back(data_arr[i]); 
        }
  
        // erases first 12 bytes off array
        data_arr.erase(data_arr.begin(), data_arr.begin() + PACKET_SIZE);
        // remove 12 from index 
        current_data_amount = current_data_amount - PACKET_SIZE;
        // ques up on packet waiting
        xQueueSend(this->packet_wait, &this->packet_size, portMAX_DELAY); 
      }
    }
    // wait 
    vTaskDelay(5);  
  }
}

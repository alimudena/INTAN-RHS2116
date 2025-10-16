#include <msp430.h>
#include <stdbool.h> 

//stop watchdog
void stop_wd();

// configure load caps --> init MSP
void init_MSP();
// toggle pin (for testing the leds attached)
void toggle_setup();
void toggle_pin();
void OFF_pin();
void ON_pin();

//LED for error happening in INTAN 
void INTAN_LED_setup();
void OFF_INTAN_LED();
void ON_INTAN_LED();


//CS for INTAN
void CS_setup();
void ON_CS_pin();
void OFF_CS_pin();

//stim_en for INTAN
void stim_en_setup();
void stim_en_ON();
void stim_en_OFF();

//Button pressed for external interaction
void button_init();
bool button_pressed();

//CS for ESP
void CS_ESP_setup();
void ON_CS_ESP_pin();
void OFF_CS_ESP_pin();



// Pin where MSP430 allows ESP32 to send data via SPI
void enable_ESP32_send_parameters_setup();
void enable_ESP32_send_parameters();
void disable_ESP32_send_parameters();


// Pin where ESP32 asks to send data via SPI

void ESP32_ask_send_parameters_setup();
bool ESP32_ask_send_parameters();

//Is ESP32 connected?
void ESP32_connected_setup();
bool ESP32_connected();


//LED for ESP32 new parameters
void ESP32_LED_setup();
void OFF_ESP32_LED();
void ON_ESP32_LED();

// end



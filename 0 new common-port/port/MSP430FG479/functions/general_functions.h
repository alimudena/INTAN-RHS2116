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
void CS_INTAN_setup();
void ON_CS_INTAN_pin();
void OFF_CS_INTAN_pin();

//stim_en for INTAN
void stim_en_setup();
void stim_en_ON();
void stim_en_OFF();

//Button pressed for external interaction
void button_init();
bool button_pressed();

//CS for ESP PARAMETERS
void CS_ESP_PARAM_setup();
void ON_CS_ESP_PARAM_pin();
void OFF_CS_ESP_PARAM_pin();

//CS for ESP ECG
void CS_ESP_ECG_setup();
void ON_CS_ESP_ECG_pin();
void OFF_CS_ESP_ECG_pin();


// Pin where MSP430 allows ESP32 to send data via SPI
void ACK_param_setup();
void ON_ACK_param();
void OFF_ACK_param();


// Pin where ESP32 asks to send data via SPI

void new_param_setup();
bool new_param_read();

//Is ESP32 connected?
void ESP32_connected_setup();
bool ESP32_connected();


//LED for ESP32 new parameters
void ESP32_LED_setup();
void OFF_ESP32_LED();
void ON_ESP32_LED();

// end



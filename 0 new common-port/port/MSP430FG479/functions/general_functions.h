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


//Timing control from ESP
void timing_control_ESP_init();
bool timing_control_ESP();


// Stimulation enabler ESP32
void stimulation_enable_ESP_init();
bool stimulation_enable_ESP();

// Control if new parameters are to be sent by ESP32
void new_parameters_ESP_init();
bool new_parameters_ESP();

// end



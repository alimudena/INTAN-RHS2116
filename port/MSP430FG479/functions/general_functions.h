#include <msp430.h>

//stop watchdog
void stop_wd();

// configure load caps --> init MSP
void init_MSP();
// toggle pin (for testing the leds attached)
void toggle_setup();
void toggle_pin();
void OFF_pin();
void ON_pin();


//CS for INTAN
void CS_setup();
void ON_CS_pin();
void OFF_CS_pin();


//stim_en for INTAN
void stim_en_setup();
void stim_en_ON();
void stim_en_OFF();

// end


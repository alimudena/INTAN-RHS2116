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


//Timer A
void timer_A();

// end


#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Necesario para uint16_t
#include "general_functions.h"

//Init MPS --> configure load caps
void init_MSP(){
    FLL_CTL0 |= XCAP14PF;                     // Configure load caps
}


//stop watchdog
void stop_wd(){
    WDTCTL = WDTPW + WDTHOLD; 
}

//*****************************************************************************
/*TOGGLE PIN*/
//*****************************************************************************

// toggle pin (for testing the leds attached)
void toggle_setup(){
    P4DIR |= BIT6;                            // Set P4.6 to output direction
}

void toggle_pin(){
    P4OUT ^= 0x40;                          // Toggle P4.6 using exclusive-OR
    
}

void OFF_pin(){
    P4OUT &= ~0x40;                          // Off   
}

void ON_pin(){                               // On 
    P4OUT |= 0x40;
}



//*****************************************************************************
/*CS for INTAN*/
//*****************************************************************************

void CS_setup(){
    P4DIR |= BIT4;                            // Set P4.4 to output direction
}

void ON_CS_pin(){                               // On 
    P4OUT |= 0x10;
}

void OFF_CS_pin(){
    P4OUT &= ~0x10;                          // Off   
}


//*****************************************************************************
/*stim_en for INTAN*/
//*****************************************************************************

void stim_en_setup(){
    P4DIR |= BIT5;                            // Set P4.4 to output direction
}

void stim_en_ON(){                               // On 
    P4OUT |= 0x20;
}

void stim_en_OFF(){
    P4OUT &= ~0x20;                          // Off   
}


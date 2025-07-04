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
/*TIMER BASIC USAGE*/
//*****************************************************************************
/*
void timer_A(uint16_t timer_A_count){
    CCTL0 |= CCIE;                             // CCR0 interrupt enabled
    CCR0 = timer_A_count;                             // Quantity counted by Timer_A before interrupting
    TACTL |= TASSEL_2 | MC_2;                  // SMCLK, continuous mode
}

*/





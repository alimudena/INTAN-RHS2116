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
/*error LED for INTAN*/
//*****************************************************************************

void INTAN_LED_setup(){
    P4DIR |= BIT3;                            // Set P4.3 to output direction
}

void OFF_INTAN_LED(){
    P4OUT &= ~0x08;                          // Off   
}

void ON_INTAN_LED(){                               // On 
    P4OUT |= 0x08;
}

//*****************************************************************************
/*CS for INTAN*/
//*****************************************************************************

void CS_INTAN_setup(){
    P4DIR |= BIT4;                            // Set P4.4 to output direction
}

void ON_CS_INTAN_pin(){                               // On 
    P4OUT |= BIT4;
}

void OFF_CS_INTAN_pin(){
    P4OUT &= ~BIT4;                          // Off   
}

//*****************************************************************************
/*CS for ESP32*/
//*****************************************************************************

void CS_ESP_PARAM_setup(){
    P4DIR |= BIT2;                            // Set P4.2 to output direction
}

void ON_CS_ESP_PARAM_pin(){                               // On 
    P4OUT |= BIT2;
}

void OFF_CS_ESP_PARAM_pin(){
    P4OUT &= ~BIT2;                          // Off   
}


void CS_ESP_ECG_setup(){
    P4DIR |= BIT7;                            // Set P4.7 to output direction
}

void ON_CS_ESP_ECG_pin(){                               // On 
    P4OUT |= BIT7;
}

void OFF_CS_ESP_ECG_pin(){
    P4OUT &= ~BIT7;                          // Off   
}


//*****************************************************************************
/* Pin where MSP430 allows ESP32 to send data via SPI*/
//*****************************************************************************
void ACK_param_setup(){
    P1DIR |= BIT1;                             // P1.1 as output
}


void ON_ACK_param(){
    P1OUT |= BIT1;
}

void OFF_ACK_param(){
    P1OUT &= ~BIT1;
}

//*****************************************************************************
/* NEW PARAMETER AVAILABLE IN ESP32*/
//*****************************************************************************
void new_param_setup(){
    P1DIR &= ~BIT4;                             // P1.4 as input
}


bool new_param_read(){
    return (P1IN & BIT4);
}


//*****************************************************************************
/*Is ESP32 connected?*/
//*****************************************************************************
void ESP32_connected_setup(){
    P1DIR &= ~BIT5;                             // P1.5 as input
}


bool ESP32_connected(){
    return (P1IN & BIT5);
}


//*****************************************************************************
/*LED new parameters from ESP32*/
//*****************************************************************************

void ESP32_LED_setup(){
    P4DIR |= BIT0;                            // Set P4.0 to output direction
}

void OFF_ESP32_LED(){
    P4OUT &= ~0x01;                          // Off   
}

void ON_ESP32_LED(){ 
    P4OUT |= 0x01;                           // On 
}


//*****************************************************************************
/*HANDSHAKE for ESP32 SPI transmissions*/
//*****************************************************************************

//Ready pin
void HSHK_READY_setup(){
    P5DIR &= ~BIT0;                            // Set P5.0 as input
}

bool HSHK_READY_value(){
    return (P5IN & BIT0); 
}

//ACK pin
void HSHK_ACK_setup(){
    P5DIR |= BIT2;                            // Set P5.2 to output direction
}

void HSHK_ACK_high(){
    P5OUT |= 0x04;                           // On 
}

void HSHK_ACK_low(){
    P5OUT &= ~0x04;                          // Off   
}

//SEND pin
void HSHK_SEND_setup(){
    P5DIR |= BIT1;                            // Set P5.1 to output direction
}

void HSHK_SEND_high(){
    P5OUT |= 0x02;                           // On 
}

void HSHK_SEND_low(){
    P5OUT &= ~0x02;                          // Off   
}


//*****************************************************************************
/*stim_en for INTAN*/
//*****************************************************************************

void stim_en_setup(){
    P4DIR |= BIT5;                            // Set P4.5 to output direction
}

void stim_en_ON(){                               // On 
    P4OUT |= 0x20;
}

void stim_en_OFF(){
    P4OUT &= ~0x20;                          // Off   
}

//*****************************************************************************
/*stim_ON for ESP32*/
//*****************************************************************************
void stim_indicator_setup(){
    P5DIR |= BIT3;                            // Set P5.3 to output direction


}

void stim_indicator_ON(){ // Off pin for indicating the stimulation is ON
    P5OUT &= ~0x08;                          // Off pin
}

void stim_indicator_OFF(){ // On pin for indicating the stimulation is OFF
    P5OUT |= 0x08;                          // ON pin
}

//*****************************************************************************
/*button_pressed for external ineraction*/
//*****************************************************************************
void button_init(){
    P1DIR &= ~BIT0;                             // P1.0 as input
}


bool button_pressed(){
    return !(P1IN & BIT0);
}

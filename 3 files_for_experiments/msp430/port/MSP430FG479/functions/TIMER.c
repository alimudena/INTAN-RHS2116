#include <msp430.h>
#include <stdbool.h> 
#include <stdio.h>

#include "TIMER.h"

void enable_interrupt_TIMER() {
    CCTL0 |= CCIE;    // Activa el bit CCIE sin alterar el resto del registro
}

void disable_interrupt_TIMER() {
    CCTL0 &= ~CCIE;   // Limpia (desactiva) el bit CCIE sin alterar el resto
}

void clear_TIMER(){
    TACTL |= TACLR;
}

void source_clock_select_TIMER(char source_clock){
    TACTL &= ~(TASSEL0|TASSEL1);
    switch (source_clock) {
        case 'E': //TACLK
            TACTL |= TASSEL_0;
            break;

        case 'A': //ACLK
            TACTL |= TASSEL_1;
            break;

        case 'S': //SMCLK
            TACTL |= TASSEL_2;
            break;

        case 'I': //INVERTED
            TACTL |= TASSEL_3;
            break;

        default:
            perror("Error: source clock TIMER not available.");
            break;  

    }
}

void operating_mode_TIMER(char operating_mode){
    TACTL &= ~(MC_0|MC_1|MC_2|MC_3);
    switch (operating_mode) {
        case 'S': //TACLK
            TACTL |= MC_0;
            break;

        case 'U': //ACLK
            TACTL |= MC_1;
            break;

        case 'C': //SMCLK
            TACTL |= MC_2;
            break;

        case 'D': //INVERTED
            TACTL |= MC_3;
            break;
        default:
            perror("Error: operating mode TIMER not available.");
            break;  
    }
}

void set_up_mode_count(){
    
}

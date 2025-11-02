#include "msp430fg479.h"
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Neccesary for  uint16_t
#include "SD16_A.h"
#include <assert.h>  // For using assert
#include <stdbool.h> 



//*****************************************************************************
/*SELECTING THE ANALOG INPUT*/
//*****************************************************************************

void setup_analog_input(int channel){
    switch (channel) {
        case 0: // A0
            P6SEL |= BIT0+BIT1;        
            break;
        case 1: // A1
            P6SEL |= BIT3+BIT4;
            break;
        case 2: // A2
            P1SEL |= BIT6+BIT7;
            break;
        case 3: // A3
            P1SEL |= BIT4+BIT5;
            break;
        case 4: // A4
            P1SEL |= BIT2+BIT3;
            break;
        default:
            perror("Error: Not available analog input chosen.");
            break;
    }
}

void select_analog_input(int channel){
   // First, clean the bits so to not have more than one channel reading
    SD16INCTL0 &= ~(SD16INCH_0 | SD16INCH_1 |SD16INCH_2 | SD16INCH_3 | SD16INCH_4);
    
    switch (channel) {
        case 0: // A0
            SD16INCTL0 |= SD16INCH_0;            
            break;
        case 1: // A1
            SD16INCTL0 |= SD16INCH_1;   
            break;
        case 2: // A2
            SD16INCTL0 |= SD16INCH_2;   
            break;
        case 3: // A3
            SD16INCTL0 |= SD16INCH_3;   
            break;
        case 4: // A4
            SD16INCTL0 |= SD16INCH_4;   
            break;
        default:
            perror("Error: Not available analog input chosen.");
            break;
    }
}


int read_analog_input(){
    return SD16MEM0;
}



//*****************************************************************************
/*CONFIGURING THE REFERENCES*/
//*****************************************************************************

void voltage_reference(char v_reference){
    // I: Internal (1.2V)
    // O: Off-chip 
    // E: External
    // First, clean the bits so to not have a mess
    SD16CTL &= ~(SD16REFON | SD16VMIDON);
    switch (v_reference) {
        case 'I':
            SD16CTL |= SD16REFON;
            break;
        case 'E':
            SD16CTL |= SD16VMIDON;
            break;
        case 'O':
        //Already cleaned up the bits
            break;
        default:
            perror("Error: Voltage reference not contemplated.");
            break;
    
    }

}


void gain_setup(int gain){
    // First, clean the bits so to not have a mess
    SD16INCTL0 &= ~(SD16GAIN_1|SD16GAIN_2|SD16GAIN_4|SD16GAIN_8|SD16GAIN_16|SD16GAIN_32);
    switch (gain) {
        case 1: //2^0
            SD16INCTL0 |= SD16GAIN_1;
            break;
        case 2: //2^1
            SD16INCTL0 |= SD16GAIN_2;
            break;
        case 4: //2^2
            SD16INCTL0 |= SD16GAIN_4;
            break;
        case 8: //2^3
            SD16INCTL0 |= SD16GAIN_8;
            break;
        case 16: //2^4
            SD16INCTL0 |= SD16GAIN_16;
            break;
        case 32: //2^5
            SD16INCTL0 |= SD16GAIN_32;
            break;
        default:
            perror("Error: Not available gain, choose 1, 2, 4, 8, 16 or 32.");
            break;
    }
}


void SD16_clk_reference(char clk_ref) {
    // Clear for not making a mess
    SD16CTL &= ~(SD16SSEL_0|SD16SSEL_1|SD16SSEL_2|SD16SSEL_3);
    switch (clk_ref) {
        case 'M':  // For 'MCLK'
                SD16CTL |= SD16SSEL_0;
            break;

        case 'S':  // For 'SMCLK'
                SD16CTL |= SD16SSEL_1;
            break;

        case 'A':  // For 'ACLK'
                SD16CTL |= SD16SSEL_2;
            break;

        case 'T': // For 'TACLK'
                SD16CTL |= SD16SSEL_3;
            break;
        default:
            perror("Error: Not available clk reference, choose M: MCLK, S: SMCLK, A: ACLK, T:TACLK.");
            break;
    }
}

void fM_dividers(int div_1, int div_2){
    SD16CTL &= ~(SD16DIV_0|SD16DIV_1|SD16DIV_2|SD16DIV_3);    
    SD16CTL &= ~(SD16XDIV_0|SD16XDIV_1|SD16XDIV_2|SD16XDIV_3);    

    switch (div_1) {
        case 1:
            SD16CTL |= SD16DIV_0;
            break;
        case 2:
            SD16CTL |= SD16DIV_1;
            break;
        case 4:
            SD16CTL |= SD16DIV_2;
            break;
        case 8:
            SD16CTL |= SD16DIV_3;
            break;

        default:
            perror("Error: Not available clk division, choose 1, 2, 4, 8.");
            break;
    }


    switch (div_2) {
        
        case 1:
            SD16CTL |= SD16XDIV_0;
            break;
        case 3:
            SD16CTL |= SD16XDIV_1;
            break;
        case 16:
            SD16CTL |= SD16XDIV_2;
            break;
        case 48:
            SD16CTL |= SD16XDIV_3;
            break;

        default:
            perror("Error: Not available clk division, choose 1, 3, 16, 48.");
            break;
    }
    
        
}

void config_OSR(int OSR){
    // Clear bytes for not making a mess
    SD16CCTL0 &= ~(SD16OSR_32|SD16OSR_64|SD16OSR_128|SD16OSR_256|SD16OSR_512|SD16OSR_1024);
    switch (OSR) {
        case 1:
            break;
        case 32:
            SD16CCTL0|=SD16OSR_32;
            break;
        case 64:
            SD16CCTL0|=SD16OSR_64;
            break;
        case 128:
            SD16CCTL0|=SD16OSR_128;
            break;
        case 256:
            SD16CCTL0|=SD16OSR_256;
            break;
        case 512:
            SD16CCTL0|=SD16OSR_512;
            break;
        case 1024:
            SD16CCTL0|=SD16OSR_1024;
            break;
        default: 
            perror("Error: Not available OSR, choose 32, 64, 128, 256, 512, 1024.");
            break;
    }
}

//*****************************************************************************
/*SELECTION OF THE CONVERSION MODE*/
//*****************************************************************************

void conversion_mode(char conv_mode){
    // C: Continuous        
    // S: Single
    // First clean the related bits so to not make a mess
    SD16CCTL0 &= ~(SD16SC | SD16SNGL);
    switch(conv_mode) {
            case 'C':
                // Do not put SD16SNGL on
                break;
            
            case 'S':
                SD16CCTL0 |= SD16SNGL;
                break;
            default:
                perror("Error: Conversion Mode not contemplated.");
                break;
    
    }    

}

void start_conversion(void){
    SD16CCTL0 |= SD16SC;                      // Set bit to start conversion
}

void stop_conversion(void){
    SD16CCTL0 &= ~SD16SC;
}




//*****************************************************************************
/*OUTPUT DATA FORMAT*/
//*****************************************************************************
void data_format(char polarity, char sign){
    /* Polarity:
        U : Unipolar
        B : Bipolar

        Sign:
        O : Offset
        C : 2's complement      
        */  
    //Clear bits to not make a mess
    SD16CCTL0 &= ~(SD16UNI | SD16DF);
    switch(polarity) {
        case 'B':
            switch (sign) {
                case 'O':
                    SD16CCTL0 &= ~(SD16UNI | SD16DF);
                    break;
                case 'C':
                    SD16CCTL0 |= SD16DF;
                    break;      
                default:
                    perror("Error: Data Format not contemplated.");
                    break;
            }
            break;
        case 'U':
            switch (sign) {
                case 'O':
                    SD16CCTL0 |= SD16UNI;
                    break;        
                case 'C':
                    perror("Error: Unipolar data format can't represent 2's complement.");
                    break;
                default:
                    perror("Error: Data Format not contemplated.");
                    break;
            }
            break;
        default:
            perror("Error: Data Format not contemplated.");
            break;
    
    }

}

//*****************************************************************************
/*INTERRUPTION CONFIGURATION*/
//*****************************************************************************
void enable_interruption_SD16A(bool enable){
    // Clear bits so to not make a mess
    SD16CCTL0 &= ~(SD16IE); 

    if (enable){
        SD16CCTL0 |= SD16IE;            //Enable interrupt        
    }

    SD16INCTL0 |= SD16INTDLY_3;         // Interrupt on 4th sample  

}

int IFG_polling(void){
    while ((SD16CCTL0 & SD16IFG)==0);       // Poll interrupt flag
    return SD16MEM0;                     // Save CH0 results (clears IFG)
}

//*****************************************************************************
/*ENTERING LOW POWER MODE*/
//*****************************************************************************

void enter_LPM(void){
    __bis_SR_register(LPM0_bits+GIE);
}
/*
void exit_LPM(void){
    __bic_SR_register_on_exit(LPM0_bits); 
}
*/


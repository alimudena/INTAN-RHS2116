#include "msp430fg479.h"
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Neccesary for  uint16_t
#include "FLL.h"
#include <assert.h>  // For using assert
#include <stdbool.h> 

//*****************************************************************************
/*CONFIGURATION FOR DEBUGGING/N*/
//*****************************************************************************
void configure_PINS_for_clk_debug(){

  P1DIR = 0x32;                             // Set P1.5,4,1 as outputs
  P1SEL = 0x32;                             // Select P1.5,4,1 as clk option
  P1SEL2 |= 0x02;
}



//*****************************************************************************
/*CONFIGURATION FOR ACLK/N*/
//*****************************************************************************

void configure_ACLK_N(int divider){
     // First, clean the bits so to not have more than one division number chosen
    FLL_CTL1 &= ~(FLL_DIV_1 | FLL_DIV_2 | FLL_DIV_4 | FLL_DIV_8);

    switch (divider) {
        case 1:
            FLL_CTL1 |= FLL_DIV_1;
            break;
        case 2:
            FLL_CTL1 |= FLL_DIV_2;
            break;
        case 4:
            FLL_CTL1 |= FLL_DIV_4;
            break;
        case 8:
            FLL_CTL1 |= FLL_DIV_8;
            break;
        default:
            perror("Error: The chosen divider for ACLK when configuring ACLK/N is not available.");
            break;
    }

}

//*****************************************************************************
/*CONFIGURATION FOR MCLK*/
//*****************************************************************************


void select_reference_MCLK(char clk_ref_MCLK){
    /*
    CLK references
        - D: DCO
        - X: XT2
        - A: LFXT1
    */
    // First, clean the bits so to not make a mess with the references
    FLL_CTL1 &= ~(SELM_DCO | SELM_XT2 | SELM_A);
    switch(clk_ref_MCLK){
        case 'D':
            FLL_CTL1 |= SELM_DCO;
            break;
        case 'X':
            FLL_CTL1 |= SELM_XT2;
            break;
        case 'A':
            FLL_CTL1 |= SELM_A;
            break;
        default:
            perror("Error: The chosen reference for MCLK is not available.");
            break;
    }    
}

void configure_N_for_MCLK (int N_MCLK) {
    /*0 --> Disable*/
    /*2 --> fMCLK=2*fACLK          1+1 to 127+1 is possible */
    /*4 --> fMCLK=4*fACLK */
    /*8 --> fMCLK=8*fACLK */
    /*16 --> fMCLK=16*fACLK */
    /*32 --> fMCLK=32*fACLK */
    /*64 --> fMCLK=64*fACLK */
    /*128 --> fMCLK=128*fACLK */
    /*X --> fMCLK=X*fACLK */
        
    if (N_MCLK>128){
            perror("Error: the chosen N for MCLK is too high.");
            return;  
    }
    // clear bits     
    SCFQCTL &= ~(SCFQ_64K|SCFQ_128K|SCFQ_256K|SCFQ_512K|SCFQ_1M|SCFQ_2M|SCFQ_4M);
    switch(N_MCLK){
        case 0://Disable DCO
            SCFQCTL |= SCFQ_M;
            break;
        case 2:
            SCFQCTL |= SCFQ_64K;
            break;
        case 4:
            SCFQCTL |= SCFQ_128K;
            break;
        case 8:
            SCFQCTL |= SCFQ_256K;
            break;
        case 16:
            SCFQCTL |= SCFQ_512K;
            break;
        case 32:
            SCFQCTL |= SCFQ_1M;
            break;
        case 64:
            SCFQCTL |= SCFQ_2M;
            break;
        case 128:
            SCFQCTL |= SCFQ_4M;
            break;
        default:
            SCFQCTL = N_MCLK;
            //perror("Error: the chosen N for MCLK is not available.");
            break;  
    }

}


//*****************************************************************************
/*CONFIGURATION FOR SMCLK*/
//*****************************************************************************
void select_reference_SMCLK(char clk_ref_SMCLK){
    /*
    CLK references
        - D: DCO
        - X: XT2
        - N: OFF
    */
    //Clear bits
    FLL_CTL1 &= ~(SELS|SMCLKOFF);
    switch (clk_ref_SMCLK) {
        case 'D':
            break;
        case 'X':
            FLL_CTL1 |= SELS;
            break;
        case 'N':
            FLL_CTL1 |= SMCLKOFF;
            break;  
        default:
            perror("Error: the chosen reference for SMCLK is not available.");
            break;  
    }
    
}





//*****************************************************************************
/*CONFIGURATION FOR LFXT1 OSCILLATOR*/
//*****************************************************************************

/*
void LFXT1_disabled();
*/
void LFXT1_internal_cap_config(int LFXT1_int_cap){
    /*0  --> XIN Cap = XOUT Cap = 0pf */
    /*10 --> XIN Cap = XOUT Cap = 10pf */
    /*14 --> XIN Cap = XOUT Cap = 14pf */
    /*18 --> XIN Cap = XOUT Cap = 18pf */
    //Clear bits 
    FLL_CTL0 &= ~(XCAP0PF|XCAP10PF|XCAP14PF|XCAP18PF);
    switch (LFXT1_int_cap) {
        case 0:
            FLL_CTL0 |= XCAP0PF;
            break;  
        case 10:
            FLL_CTL0 |= XCAP10PF;
            break;  
        case 14:
            FLL_CTL0 |= XCAP14PF;
            break;  
        case 18:
            FLL_CTL0 |= XCAP18PF;
            break;  
        default:
            perror("Error: the chosen internal capacitance for LFXT1 is not available.");
            break;  
    }
}    

void LFXT1_working_mode(char Low_High_frequency_mode){
    //Clear bits
    FLL_CTL0 &= ~(XTS_FLL);
    switch (Low_High_frequency_mode) {
        case 'L':
            break;
        case 'H':
            FLL_CTL0 |= XTS_FLL;
            break;
         default:
            perror("Error: the working mode for LFXT1 is not possible.");
            break;                    
    }
}

void protection_cpu_required(bool osc_ext, int f_osc_ext){
    // osc_ext: True if there is an external oscillator
    // f_osc_ext: frequency of the external oscillator
    //Clear bits
    FLL_CTL0 &= ~(XT1OF);
    // If the frequency of the external oscillator is below 450kHz prevent the CPU of being clocked from the external frequency
    if (osc_ext) {
        if (f_osc_ext < 450){
            FLL_CTL0 |= XT1OF;
        }        
    }
}

//*****************************************************************************
/*CONFIGURATION FOR XT2 OSCILLATOR*/
//*****************************************************************************

void LFXT2_disable(bool LFXT2_disabled){
    // Clear bits
    FLL_CTL1 &= ~(XT2OFF);
    if (LFXT2_disabled){
        FLL_CTL1 |= XT2OFF;        
    }
}

//*****************************************************************************
/*CONFIGURATION FOR DCO*/
//*****************************************************************************

void configuring_DCO(bool DCOPLUS_on, int D_val){
    // First clean the bits

    FLL_CTL0 &= ~(DCOPLUS);
    SCFI0 &= ~(FLLD_1 | FLLD_2 | FLLD_4 | FLLD_8);

    if (DCOPLUS_on){
        FLL_CTL0 |= DCOPLUS;
        switch (D_val) {
            case 1:
                SCFI0 |= FLLD_1;
                break;
            case 2:
                SCFI0 |= FLLD_2;
                break;
            case 4:
                SCFI0 |= FLLD_4;
                break;
            case 8:
                SCFI0 |= FLLD_8;
                break;
            default:
                perror("Error: Not abailable configuration for D in DCOPLUS");
                break;
        }
    }
}

void DCO_f_range(int DCO_range){
    /*2 --> fDCOCLK =   1.4-12MHz*/
    /*3 --> fDCOCLK =   2.2-17Mhz*/
    /*4 --> fDCOCLK =   3.2-25Mhz*/
    /*8 --> fDCOCLK =     5-40Mhz*/
    // First clean bits
    SCFI0 &= ~(FN_2|FN_3|FN_4|FN_8);
    switch(DCO_range){
        case 0:

        case 2:
            SCFI0 |= FN_2;
            break;
        case 3:
            SCFI0 |= FN_3;  
            break;
        case 4:
            SCFI0 |= FN_4;
            break;
        case 8:
            SCFI0 |= FN_8;
            break;
        default:
            perror("Error: Not abailable range for DCO frequency range");
        break;
    }
}


void initClockTo8MHz()
{
    LFXT1_working_mode('L');
    LFXT1_internal_cap_config(18);
    DCO_f_range(4);
    configuring_DCO(true, 2);
    configure_N_for_MCLK(121);
    select_reference_MCLK('D');
    select_reference_SMCLK('D');
    configure_ACLK_N(1);
    LFXT2_disable(false);
}


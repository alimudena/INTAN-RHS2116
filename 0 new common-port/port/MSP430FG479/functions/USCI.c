#include "msp430fg479.h"
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Neccesary for  uint16_t
#include "SD16_A.h"
#include <assert.h>  // For using assert
#include <stdbool.h> 
#include <math.h>


//*****************************************************************************
/*USCI INITIALIZATION AND RESET FUNCTIONS*/
//*****************************************************************************

void USCI_setup(){
    //Set the bit UCSWRST in register USCI_A0 Control Register 1 
    UCA0CTL1 |= UCSWRST;

}

void USCI_init(){
    //Reset the bit UCSWRST in register USCI_A0 Control Register 1 
    UCA0CTL1 &= ~(UCSWRST);
}


void init_UART_GPIO()
{
    P2SEL = BIT4 | BIT5;                      // P2.5 = RXD, P2.4=TXD
}


void USCI_clk_ref(char clk_ref){
    /*clk_ref:
        U --> UCLK
        A --> ACLK
        S --> SMCLK        
    */
    UCA0CTL1 &= ~(UCSSEL_0|UCSSEL_1|UCSSEL_2|UCSSEL_3);
    switch (clk_ref) {
        case 'U': //UCLK
            UCA0CTL1|=UCSSEL_0;
            break;
        case 'A': //ACLK
            UCA0CTL1|=UCSSEL_1;
            break;
        case 'S': //SMCLK
            UCA0CTL1|=UCSSEL_2;
            break;
        default:
            perror("Error: Not available clk reference in USART.");
            break;
    }
}


void USCI_interrupt_enable(bool enable_USCI_interr_rx, bool enable_USCI_interr_tx){
    IE2 &= ~(UCA0RXIE|UCA0TXIE);
    if (enable_USCI_interr_rx){
        IE2 |= UCA0RXIE;                 // Enable USCI_A0 RX interrupt
    }else if (enable_USCI_interr_tx){
        IE2 |= UCA0TXIE;                 // Enable USCI_A0 TX interrupt

    };
}


//*****************************************************************************
/*UART RELATED FUNCTIONS*/
//*****************************************************************************

//Character format decision
void character_format_sel(bool parity_enable, char parity_type, int num_data_bit, int num_stop_bit, char first_Byte_sent){
    //parity enable:
        //True --> parity ON
        //False --> parity OFF            
    //if parity enable:
        //parity_type:
            //E --> even (par)
            //O --> odd (impar)
    //num_data_bit: quantity of data bits available (character length)
        //7 or 8
    //num_stop_bit: stop bit select, one at least
        //1 or 2
    //first_byte_sent: To choose between MSB or LSB 
        //M: MSB first
        //L: LSB first
    
    // first clear all bits
    UCA0CTL0 &= ~(UCPEN|UCPAR|UCMSB|UC7BIT|UCSPB);
    //parity enable
    if (parity_enable){
        UCA0CTL0 |= UCPEN;
        switch (parity_type) {
            case 'E':
                UCA0CTL0 |= UCPAR;
                break;
            case 'O':
                break;
            default:
                perror("Error: Not available parity in UART selection.");
                break;
        }
    }

    switch (num_data_bit) {
        case 7:
            UCA0CTL0 |= UC7BIT;
            break;
        case 8:
            break;
        default:
            perror("Error: Not available number of data bits in UART selection.");
            break;
    }
    
    switch (num_stop_bit) {
        case 1:
            break;
        case 2:
            UCA0CTL0 |= UCSPB;
            break;            
        default:
            perror("Error: Not available number of stop bits in UART selection.");
            break;
    }
    
    switch (first_Byte_sent){
        case 'L':
            break;
        case 'M':
            UCA0CTL0 |= UCMSB;        
            break;
        default:
            perror("Error: Not available first Byte  in UART selection.");
            break;
    }
}

void USCI_mode_sel(char USCI_mode){
    //U --> Uart
    //I --> IDLE-LINE MULTIPROCESSOR MODE
    //D --> ADDRESS-BIT MULTIPROCESSOR MODE
    //A --> UART MODE WITH AUTOMATIC BAUD RATE DETECTION
    UCA0CTL0 &= ~(UCMODE0|UCMODE1);
    switch (USCI_mode) {
        case 'U': //UART
            break;
        case 'I': //IDLE-LINE MULTIPROCESSOR MODE
            UCA0CTL0 |= UCMODE0;
            break;
        case 'D': //ADDRESS-BIT MULTIPROCESSOR MODE
            UCA0CTL0 |= UCMODE1;
            break;
        case 'A'://UART MODE WITH AUTOMATIC BAUD RATE DETECTION
            UCA0CTL0 |= UCMODE1+UCMODE0;
            break;
        default:
            perror("Error: Not available UART mode.");
            break;
    }
}


void data_to_transmit(uint8_t data){
    while (!(IFG2&UCA0TXIFG));                // The register for the data only can be written if the flag UCA0TXIFG is up
    UCA0TXBUF = data;
}
/*
void UART_baudrate_generation(float ref_frec, float baudrate_wanted){
    UCA0MCTL  &= ~(UCOS16);
    float N = ref_frec/baudrate_wanted;
    char BR = 'L'; //baudrate indicator, if N >= 16 -- > High, else --> low --> for the selection of the configuration

    if (N>=16){
        UCA0MCTL |= UCOS16;
        BR = 'H';
    }

    switch (BR) {
        case 'L':
            int UCBRX = int(N);

            //UCBRX = UCA0BR0 + UCA0BR1*256;
            int prod = int(UCBRX/256);
            UCA0BR1 = prod;
            UCA0BR0 = UCBRX-prod*256;




            break;

        default:
            perror("Error:Something went wrong in selecting baudrate.");
            break; 
    }

}
*/

void UART_baudrate_generation(int32_t BRCLK_freq, int32_t baudrate){
    switch ((int)BRCLK_freq) {
        case (int)32768:
            switch ((int)baudrate) {
                case (int)1200:
                    UCA0BR0 = 27;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_2;                       
                    break;

                case (int)2400:
                    UCA0BR0 = 13;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_6;                       
                    break;

                case (int)4800:
                    UCA0BR0 = 6;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_7;                       
                    break;

                case (int)9600:
                    UCA0BR0 = 3;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_3;                       
                    break;

                default:
                    perror("Error: Something went wrong in selecting baudrate.");
                    break; 

            }
        break;
        
        case (int)1000000:
            switch ((int)baudrate) {
                case (int)9600:
                    UCA0BR0 = 104;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_1;                       
                    break;

                case (int)19200:
                    UCA0BR0 = 52;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_0;                       
                    break;

                case (int)38400:
                    UCA0BR0 = 26;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_0;                       
                    break;

                case (int)57600:
                    UCA0BR0 = 17;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_3;                       
                    break;

                case (int)115200:
                    UCA0BR0 = 8;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_6;                       
                    break;

                
                default:
                    perror("Error: Something went wrong in selecting baudrate.");
                    break; 
            }
        break;

        case (int)1048576:
            switch ((int)baudrate) {
                case (int)9600:
                    UCA0BR0 = 109;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_2;                       
                    break;

                case (int)19200:
                    UCA0BR0 = 54;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_5;                       
                    break;

                case (int)38400:
                    UCA0BR0 = 27;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_2;                       
                    break;

                case (int)57600:
                    UCA0BR0 = 18;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_1;                       
                    break;

                case (int)115200:
                    UCA0BR0 = 9;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_1;                       
                    break;

                
                default:
                    perror("Error: Something went wrong in selecting baudrate.");
                    break; 
            }
        break;

        case (int)4000000:
            switch ((int)baudrate) {
                case (int)9600:
                    UCA0BR0 = 160;                              
                    UCA0BR1 = 1;                              
                    UCA0MCTL = UCBRS_6;                       
                    break;

                case (int)19200:
                    UCA0BR0 = 208;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_3;                       
                    break;

                case (int)38400:
                    UCA0BR0 = 104;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_1;                       
                    break;

                case (int)57600:
                    UCA0BR0 = 69;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_4;                       
                    break;

                case (int)115200:
                    UCA0BR0 = 34;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_6;                       
                    break;

                case (int)230400:
                    UCA0BR0 = 17;                              
                    UCA0BR1 = 0;                              
                    UCA0MCTL = UCBRS_3;                       
                    break;
                
                default:
                    perror("Error: Something went wrong in selecting baudrate.");
                    break; 
            }
        break;

        case (int)8000000:
                    switch ((int)baudrate) {
                        case (int)9600:
                            UCA0BR0 = 65;                              
                            UCA0BR1 = 3;                              
                            UCA0MCTL = UCBRS_2;                       
                            break;

                        case (int)19200:
                            UCA0BR0 = 163;                              
                            UCA0BR1 = 1;                             
                            UCA0MCTL = UCBRS_6;                       
                            break;

                        case (int)38400:
                            UCA0BR0 = 208;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_3;                       
                            break;

                        case (int)57600:
                            UCA0BR0 = 138;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_7;                       
                            break;

                        case (int)115200:
                            UCA0BR0 = 69;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_4;                       
                            break;

                        case (int)230400:
                            UCA0BR0 = 34;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_6;                       
                            break;
                        
                        case (int)460800:
                            UCA0BR0 = 17;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_3;                       
                            break;

                        default:
                            perror("Error: Something went wrong in selecting baudrate.");
                            break; 
                    }
        break;
        case (int)12000000:
                    perror("Error: MSP430FG479 does not have this frequency.");

                    switch ((int)baudrate) {
                        case (int)9600:
                            UCA0BR0 = 226;                              
                            UCA0BR1 = 4;                              
                            UCA0MCTL = UCBRS_0;                       
                            break;

                        case (int)19200:
                            UCA0BR0 = 113;                              
                            UCA0BR1 = 2;                             
                            UCA0MCTL = UCBRS_0;                       
                            break;

                        case (int)38400:
                            UCA0BR0 = 56;                              
                            UCA0BR1 = 1;                              
                            UCA0MCTL = UCBRS_4;                       
                            break;

                        case (int)57600:
                            UCA0BR0 = 208;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_2;                       
                            break;

                        case (int)115200:
                            UCA0BR0 = 104;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_1;                       
                            break;

                        case (int)230400:
                            UCA0BR0 = 52;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_0;                       
                            break;
                        
                        case (int)460800:
                            UCA0BR0 = 26;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_0;                       
                            break;

                        default:
                            perror("Error: Something went wrong in selecting baudrate.");
                            break; 
                    }
        break;


        case (int)16000000:
                    perror("Error: MSP430FG479 does not have this frequency.");
                    switch ((int)baudrate) {
                        case (int)9600:
                            UCA0BR0 = 130;                              
                            UCA0BR1 = 6;                              
                            UCA0MCTL = UCBRS_6;                       
                            break;

                        case (int)19200:
                            UCA0BR0 = 65;                              
                            UCA0BR1 = 3;                             
                            UCA0MCTL = UCBRS_2;                       
                            break;

                        case (int)38400:
                            UCA0BR0 = 160;                              
                            UCA0BR1 = 1;                              
                            UCA0MCTL = UCBRS_6;                       
                            break;

                        case (int)57600:
                            UCA0BR0 = 21;                              
                            UCA0BR1 = 1;                              
                            UCA0MCTL = UCBRS_7;                       
                            break;

                        case (int)115200:
                            UCA0BR0 = 138;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_7;                       
                            break;

                        case (int)230400:
                            UCA0BR0 = 69;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_4;                       
                            break;
                        
                        case (int)460800:
                            UCA0BR0 = 34;                              
                            UCA0BR1 = 0;                              
                            UCA0MCTL = UCBRS_6;                       
                            break;

                        default:
                            perror("Error: Something went wrong in selecting baudrate.");
                            break; 
                    }
                break;

    }

}


// IrDA encoding enable
void IrDA_enable(bool IrDA_enabled){
    UCA0IRTCTL &= ~(UCIREN);
    if (IrDA_enabled){
        UCA0IRTCTL |= UCIREN;
    }

}

void IrDA_pulse_config(char IrDA_clk_ref, int IrDA_Transmit_Pulse_Length){
/* UCIRTXPL5 --> IRDA Transmit Pulse Length 5 */
/* UCIRTXPL4 --> IRDA Transmit Pulse Length 4 */
/* UCIRTXPL3 --> IRDA Transmit Pulse Length 3 */
/* UCIRTXPL2 --> IRDA Transmit Pulse Length 2 */
/* UCIRTXPL1 --> IRDA Transmit Pulse Length 1 */
/* UCIRTXPL0 --> IRDA Transmit Pulse Length 0 */
    UCA0IRTCTL &= ~(UCIRTXCLK|UCIRTXPL5|UCIRTXPL4|UCIRTXPL3|UCIRTXPL2|UCIRTXPL1|UCIRTXPL0);    
    switch (IrDA_clk_ref) {
        case 'I': // BITCLK16 
            UCA0IRTCTL |= UCIRTXCLK;
            break;
        case 'R': // BRCLK 
            break;
        default:
            perror("Error: Not available IrDA clk reference.");
            break; 
    }
    switch (IrDA_Transmit_Pulse_Length) {
        case 5:
            UCA0IRTCTL |= UCIRTXPL5;        
            break;
        case 4:
            UCA0IRTCTL |= UCIRTXPL4;        
            break;
        case 3:
            UCA0IRTCTL |= UCIRTXPL3;        
            break;
        case 2:
            UCA0IRTCTL |= UCIRTXPL2;        
            break;
        case 1:
            UCA0IRTCTL |= UCIRTXPL1;        
            break;
        case 0:
            UCA0IRTCTL |= UCIRTXPL0;        
            break;
        default:
            perror("Error: Not available IrDA clk divider proportion.");
            break; 
    }    

}

void IrDA_detect_value(int H_L_detect){
    UCA0IRRCTL &= ~(UCIRRXPL);
    if (H_L_detect==0){
        UCA0IRRCTL |= UCIRRXPL;
    }
}


void IrDA_decoding_filter(bool IrDA_dec_filter_enabled, int IrDA_Receive_Filter_Length){
/*UCIRRXFL5 -->  IRDA Receive Filter Length 5 */
/*UCIRRXFL4 -->  IRDA Receive Filter Length 4 */
/*UCIRRXFL3 -->  IRDA Receive Filter Length 3 */
/*UCIRRXFL2 -->  IRDA Receive Filter Length 2 */
/*UCIRRXFL1 -->  IRDA Receive Filter Length 1 */
/*UCIRRXFL0 -->  IRDA Receive Filter Length 0 */
    UCA0IRRCTL &= ~(UCIRRXFE|UCIRRXFL5|UCIRRXFL4|UCIRRXFL3|UCIRRXFL2|UCIRRXFL1|UCIRRXFL0);
    if (IrDA_dec_filter_enabled){
        UCA0IRRCTL |= UCIRRXFE;
    }
    switch (IrDA_Receive_Filter_Length) {
        case 5:
            UCA0IRRCTL |= UCIRRXFL5;
            break;
        case 4:
            UCA0IRRCTL |= UCIRRXFL4;
            break;
        case 3:
            UCA0IRRCTL |= UCIRRXFL3;
            break;
        case 2:
            UCA0IRRCTL |= UCIRRXFL2;
            break;
        case 1:
            UCA0IRRCTL |= UCIRRXFL1;
            break;
        case 0:
            UCA0IRRCTL |= UCIRRXFL0;
            break;
        default:
            perror("Error: Not available IrDA decoder digital filter configuration.");
            break; 
    }
}

//*****************************************************************************
/*SPI RELATED FUNCTIONS*/
//*****************************************************************************

void USCI_SPI_pin_setup(){
    P2SEL |= BIT4+BIT5;                          // P2.4,2.5 option select
    P3SEL |= BIT0;                            // P3.0 option select
  }

void SPI_mode_config(char Master_Slave){
    /*
    Selection if the controller works as Master (M) or Slave (S)*/
    UCA0CTL0 &= ~(UCMST);
    switch (Master_Slave) {
        case 'M':
            UCA0CTL0 |= UCMST;
            break;

        case 'S':
            break;

        default:
            perror("Error: Not available SPI mode configuration.");
            break;

    }
}

void SPI_char_format(int SPI_length, char first_Byte_sent){
    UCA0CTL0 &= ~(UC7BIT|UCMSB);
    switch (SPI_length) {
        case 7:
            UCA0CTL0 |= UC7BIT;
            break;
        
        case 8:
            break;

        default:
            perror("Error: Not available SPI char length format configuration.");
            break;

    }


    switch (first_Byte_sent) {
        case 'M':
            UCA0CTL0 |= UCMSB;
            break;
        
        case 'L':
            break;

        default:
            perror("Error: Not available SPI char first byte sent format configuration.");
            break;

    }
}

void SPI_clk_division(int clk_div){
    //UCBRX = UCA0BR0 + UCA0BR1*256;
    int prod = round(clk_div/256);
    UCA0BR1 = prod;
    UCA0BR0 = clk_div-prod*256;

    //reset the modulation value because with USCIA0 is recommended 
    UCA0MCTL = 0;                             // No modulation
}


void SPI_clk_polarity_phase(char inactive_state, char data_on_clock_edge){
    UCA0CTL0  &= ~(UCCKPH|UCCKPL);    //3-pin, 8-bit SPI master

    switch (inactive_state) {
        case 'L': // clock polarity inactive low

            break;
        
        case 'H': // clock polarity inactive high
            UCA0CTL0 |= UCCKPL;
            break;
        
        default: 
            perror("Error: Not available SPI clock polarity.");
            break;
    }

    switch (data_on_clock_edge) {
        case 'H': //data cHanged on the first UCLK edge and captured on the following edge
            break;
        case 'A': //data cAptured on the first UCLK edge and changed on the following edge
            UCA0CTL0 |= UCCKPH;
            break;
        default:
            perror("Error: Not available SPI clock polarity.");
            break;

    }
}



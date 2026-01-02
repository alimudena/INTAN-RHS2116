#include <stdbool.h> 
#include <stdint.h>

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

typedef struct{
    /*
		- A: Active
		- L: Low Power
            - 0: LPM0
            - 1: LPM1
            - 2: LPM2 
            - 3: LPM3
    */
    char operating_mode;

    /*clk_ref:
        U --> UCLK
        A --> ACLK
        S --> SMCLK        
    */
    char USCI_clk_ref_sel;

    /*Baudrate and BRCLK_freq combinations:

    BRCLK_freq      32768     1000000   1048576   4000000   8000000   12000000  16000000
    ------------------------------------------------------------------------------
                    1200      
                    2400      
                    4800      
                    9600       9600       9600      9600      9600      9600      9600
                     	       19200      19200     19200     19200     19200     19200
                               38400      38400     38400     38400     38400     38400
                               57600      57600     57600     57600     57600     57600
                               115200     115200    115200    115200    115200    115200
                                                    230400    230400    230400    230400
                                                              460800    460800    460800
    */ 
    int32_t BRCLK_freq;
    int32_t baudrate;

    //parity enable:
        //True --> parity ON
        //False --> parity OFF            
    //if parity enable:
        //parity_type:
            //E --> even (par)
            //O --> odd (impar)
    bool parity_enable;
    char parity_type;

    //num_data_bit: quantity of data bits available (character length)
        //7 or 8 
    int num_data_bit;

    //num_stop_bit: stop bit select, one at least
        //1 or 2
    int num_stop_bit;

    //first_byte_sent: To choose between MSB or LSB 
        //M: MSB first
        //L: LSB first
    char first_Byte_sent;

    //U --> Uart
    //I --> IDLE-LINE MULTIPROCESSOR MODE
    //D --> ADDRESS-BIT MULTIPROCESSOR MODE
    //A --> UART MODE WITH AUTOMATIC BAUD RATE DETECTION
    char USCI_mode;
    
} UART_config_struct;

#endif

void setup_UART(UART_config_struct UART_config);

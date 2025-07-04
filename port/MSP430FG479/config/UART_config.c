#include "UART_config.h"
#include "../functions/USCI.h"
#include "../functions/system_config.h"

void setup_UART(UART_config_struct UART_config){
    //******************************************************************************
    // UART Initialization *********************************************************
    //******************************************************************************

    /*
		- A: Active
		- L: Low Power
            - 0: LPM0
            - 1: LPM1
            - 2: LPM2 
            - 3: LPM3
    */
    UART_config.operating_mode = 'A';
    select_operating_mode(UART_config.operating_mode, 0);



    //Init UART related peripherics
    init_UART_GPIO();

    //Config UART
    /*clk_ref:
        U --> UCLK
        A --> ACLK
        S --> SMCLK        
    */
    UART_config.USCI_clk_ref_sel = 'S';
    USCI_clk_ref(UART_config.USCI_clk_ref_sel);    


    //Reference frequence chosen for UART operation
    UART_config.BRCLK_freq = 8000000; //Source clock frequency
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
    UART_config.baudrate = 230400;
    UART_baudrate_generation(UART_config.BRCLK_freq, UART_config.baudrate);
    USCI_init();                     // **Initialize USCI state machine**


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
    UART_config.parity_enable = false;
    UART_config.parity_type = 'O';
    UART_config.num_data_bit = 8;
    UART_config.num_stop_bit = 1;
    UART_config.first_Byte_sent = 'L';
    character_format_sel(UART_config.parity_enable, UART_config.parity_type, UART_config.num_data_bit, UART_config.num_stop_bit, UART_config.first_Byte_sent);

    //U --> Uart
    //I --> IDLE-LINE MULTIPROCESSOR MODE
    //D --> ADDRESS-BIT MULTIPROCESSOR MODE
    //A --> UART MODE WITH AUTOMATIC BAUD RATE DETECTION
    UART_config.USCI_mode = 'U';

    USCI_mode_sel(UART_config.USCI_mode);



}


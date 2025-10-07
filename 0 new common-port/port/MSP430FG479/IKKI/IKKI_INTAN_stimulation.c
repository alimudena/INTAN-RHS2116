//                   MSP430x47x
//                 -----------------
//             /|\|              XIN|-
//              | |                 |  32kHz xtal
//              --|RST          XOUT|-
//                |                 |
//                |   14        P4.4|--> CS for INTAN
//                |   13        P4.5|--> stim_en for INTAN
//                |   12        P4.6|--> LED
//                |   15        P4.3|--> LED for INTAN bug
//                |                 |
//                |                 |               VCC (3.3V)     
//                |                 |                   |    
//                |                 |                   / (resistencia externa de pull-up, 10kΩ)    
//                |                 |                   |    
//                |   58        P1.0|--> Botón         P1.0
//                |                 |                   |    
//                |                 |               [ BOTÓN ]
//                |                 |                   |    
//                |                 |                  GND
//                |                 |
//                |  ---  CLK  ---  |
//                |                 |
//                |             P1.1|--> MCLK = 8Mhz  --> 57 (referencia DCO)
//                |             P1.4|--> SMCLK = 8MHz --> 54 (referencia DCO)
//                |             P1.5|--> ACLK = 32kHz --> 51
//                |                 |
//                |  ---  UART ---  |
//                |  75         P2.5|<------- Receive Data (UCA0RXD) 
//                |  76         P2.4|-------> Transmit Data (UCA0TXD) 
//                |                 |
//                |                 |
//                |  ---  SPI  ---  |
//                |                 |
//                |  76         P2.4|-> Data Out (UCA0SIMO) 
//                |  75         P2.5|<- Data In (UCA0SOMI)
//                |  41         P3.0|-> Serial Clock Out (UCA0CLK)
//                |                 |
//                |  ---  SD16 ---  |
//                |                 |
//                |             P1.5|<------- A3+: EEG1 positive input --> 51
//                |             P1.4|<------- A3-: EEG1 negative input --> 54
//                |                 |
//                |             P1.7|<------- A2+: EEG2 positive input --> 49
//                |             P1.6|<------- A2-: EEG2 negative input --> 50
//                |                 |
//                |             P6.3|<------- A1+: EEG3 positive input --> 64
//                |             P6.4|<------- A1-: EEG3 negative input --> 63
//                |                 |
//                |             P6.0|<------- A0+: ECG positive input --> 67
//                |             P6.1|<------- A0-: ECG negative input --> 66
//                |                 |
//                |             P1.3|<------- A4+: BATT positive input --> 55
//                |             P1.2|<------- A4-: BATT negative input --> 56
//                |                 |


#include "msp430fg479.h"
#include <msp430.h>

#include "../functions/system_config.h"
#include "../functions/general_functions.h"
#include "../functions/SD16_A.h"
#include "../functions/FLL.h"

#include "IKKI_MAC.h"

#include "../config/SD16A_config.h"
#include "../config/SPI_config.h"
#include "../config/UART_config.h"
#include "../config/clk_config.h"

#include "../../../common/INTAN_config.h"
#include "../../../common/register_macro.h"


volatile uint8_t high_word = 0x00;
volatile uint8_t low_word = 0xFF;
bool high_or_low = true;
int i;
uint16_t my_register = 0;



#define UART_USAGE    false //True: UART, False: SPI
#define TEST_CLK      false
#define SENSE_OR_STIM 3 //1: SENSE interruptions 2: SENSE while 3:STIM 

CLK_config_struct CLK_config;
SD16A_config_struct SD16A_configuration;
UART_config_struct UART_config;
SPI_config_struct SPI_config;
INTAN_config_struct INTAN_config;

bool stimulate = false;

void config_SPI_stim(SPI_config_struct* SPI_config){
  SPI_config->Master_Slave = 'M';       //M: Master, S: Slave
  SPI_config->inactive_state = 'L';     // clock polarity inactive low
  //data cHanged on the first UCLK edge and captured on the following edge
  //data cAptured on the first UCLK edge and changed on the following edge
  SPI_config->data_on_clock_edge = 'A'; 
  SPI_config->SPI_length = 8;
  SPI_config->first_Byte_sent = 'M'; //M: MSB, L: LSB
    /*clk_ref:
      U --> UCLK
      A --> ACLK
      S --> SMCLK
  */
  SPI_config->clk_ref_SPI = 'S';
  SPI_config->clk_div = 2;
  SPI_config->enable_USCI_interr_rx = false;
  SPI_config->enable_USCI_interr_tx = false;
}

int main(void) {
    general_setup(CLK_config);
    init_MSP();

    //************************** CLK configuration *****************************
    
    setup_CLK(CLK_config);

    //************************** LED configuration *****************************
    toggle_setup(); // Setup P4.6 for LED output


    config_SPI_stim(&SPI_config);

    SPI_setup(&SPI_config);

    INTAN_LED_setup(); // Setup P4.3 for LED output
    OFF_INTAN_LED();

    CS_setup();
    ON_CS_pin();
    stim_en_setup();
    button_init();

    stim_en_OFF();

    while(1){
      bool next_stim = button_pressed();
      if(next_stim == true){

        initialize_INTAN(&INTAN_config);
        check_intan_SPI_array(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        disable_C2(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        clear_command(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        minimum_power_disipation(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        stimulation_disable(&INTAN_config);
        read_command(&INTAN_config, REGISTER_1, 'G');
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        INTAN_config.step_DAC = 5000;
        stim_PNBIAS_configuration(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);

        /*
          REGISTRO 0X22 NO CONSIGO ESCRIBIR EN ÉL
        */
        // stim_step_DAC_configuration(&INTAN_config);
        // send_SPI_commands(&INTAN_config);
        // __delay_cycles(CLK_10_CYCLES);

        
        stimulation_on(&INTAN_config);
        stimulation_polarity(&INTAN_config);
        uint8_t stim_channel = 0;
        uint8_t neg_current_mag = 100;
        uint8_t neg_current_trim = 128;
        uint8_t pos_current_mag = 200;
        uint8_t pos_current_trim = 128;
        stim_current_channel_configuration(&INTAN_config, stim_channel, neg_current_trim, neg_current_mag, pos_current_trim, pos_current_mag);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);



        /* READ OR CLEAR - ESCOGER*/
        enable_M_flag(&INTAN_config);
        enable_U_flag(&INTAN_config);
        read_command(&INTAN_config, REGISTER_1, 'G');
        disable_M_flag(&INTAN_config);
        disable_U_flag(&INTAN_config);
        send_SPI_commands(&INTAN_config);
        __delay_cycles(CLK_10_CYCLES);


        while(1){
          if (stimulate){
            stimulation_enable(&INTAN_config);
            __delay_cycles(CLK_30_S_CYCLES);
          }else{
            stimulation_disable(&INTAN_config);
            __delay_cycles(CLK_5_M_CYCLES);
          }
          stimulate = !stimulate;
          __delay_cycles(CLK_1_S_CYCLES);
        }


        while(next_stim == true){
          next_stim = button_pressed();
        }
      }
    }
}





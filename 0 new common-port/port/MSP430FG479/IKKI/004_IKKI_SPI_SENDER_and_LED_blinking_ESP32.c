//                   MSP430x47x
//                 -----------------
//             /|\|              XIN|-
//              | |                 |  32kHz xtal
//              --|RST          XOUT|-
//                |                 |
//                |  14         P4.4|--> CS for INTAN
//                |  13         P4.5|--> stim_en for INTAN
//                |  12         P4.6|--> LED
//                |  15         P4.3|--> LED for INTAN bug
//                |                 |
//                |                 |               VCC (3.3V)     
//                |                 |                   |    
//                |                 |                   / (resistencia externa de pull-up, 10kΩ)    
//                |                 |                   |    
//                |  58         P1.0|--> Botón         P1.0
//                |                 |                   |    
//                |                 |               [ BOTÓN ]
//                |                 |                   |    
//                |                 |                  GND
//                |                 |
//                |                 |
//                |                 |
//                |  --- ESP32 ---  |                                           ESP32            
//                |  16         P4.2|--> CS for ESP32 ------------------------> - GPIO 15: A5
//                |  76         P2.4|-> Data Out (UCA0SIMO) ------------------> - GPIO 13: D13
//                |  75         P2.5|<- Data In (UCA0SOMI) -------------------> - GPIO 14: A4
//                |  41         P3.0|-> Serial Clock Out (UCA0CLK) -----------> - GPIO 12: D12
//                |  57         P1.1|-> Alert ESP32 change -------------------> - GPIO 2:  RX
//                |                 |
//                |                 |
//                |                 |
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


        //************************** SPI comms configuration *****************************
        config_SPI_stim(&SPI_config);

        SPI_setup(&SPI_config);

        //************************** INTAN programming setup ************************** 
        INTAN_LED_setup(); // Setup P4.3 for LED output
        OFF_INTAN_LED();

        CS_setup();
        ON_CS_pin();
        stim_en_setup();
        stim_en_OFF();

        //************************** External button setup  ************************** 
        button_init();

        //************************** ESP32 timing control setup  ************************** 
        timing_control_ESP_init();

        //************************** ESP32 communication setup ************************** 
        CS_ESP_setup();
        ON_CS_ESP_pin();

        bool next_stim = button_pressed(); 
        volatile uint8_t RX;
        bool toggled = false;

        while(1){
                while(!next_stim){
                        next_stim = button_pressed();
                }


                // It is always sending data to the ESP32, if the timing pin is enabled then, the incorporated led will switch
                OFF_CS_ESP_pin();
                /*
                        We try only to send one value by SPI to the ESP32 every time we press the button
                */
                while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
                UCA0TXBUF = 0xAA;
                while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
                RX = UCA0RXBUF;
                ON_CS_ESP_pin();

                // LED control of on and off - pasará a ser el envío a través del intan
                if(timing_control_ESP()){
                        if(!toggled){
                                toggle_pin();
                                // LED control of on and off
                                toggled = true;
                                
                        }
                }else{
                        toggled = false;
                }
        }     
}





//                   MSP430x47x
//                 -----------------
//             /|\|              XIN|-
//              | |                 |  32kHz xtal
//              --|RST          XOUT|-
//                |                 |
//                |   14        P4.4|--> CS for INTAN
//                |   13        P4.5|--> stim_en for INTAN
//                |   12        P4.6|--> LED
//                |                 |
//                |  ---  CLK  ---  |
//                |                 |
//                |             P1.1|--> MCLK = 8Mhz  --> 57 (referencia DCO)
//                |             P1.4|--> SMCLK = 8MHz --> 54 (referencia DCO)
//                |             P1.5|--> ACLK = 32kHz --> 51
//                |                 |
//                |  ---  UART ---  |
//                |             P2.5|<------- Receive Data (UCA0RXD) --> 75
//                |             P2.4|-------> Transmit Data (UCA0TXD) --> 76
//                |                 |
//                |                 |
//                |  ---  SPI  ---  |
//                |                 |
//                |             P2.4|-> Data Out (UCA0SIMO) --> 76
//                |             P2.5|<- Data In (UCA0SOMI) --> 75
//                |             P3.0|-> Serial Clock Out (UCA0CLK) --> 41
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


/*
State machine for SD16: working with interruptions
1. Send Channel ID being sampled
2. Sample
3. Send sample (x2)

State changes:
1 - 2 : interruption SD16 ON
        interruption UART OFF
2 - 3 : interruption SD16 OFF
        interruption UART ON
3 - 1 : interruption SD16 OFF
        interruption UART ON

*/

/*
State machine for INTAN configuration: working with polling
1. Update register and value to be sent
2. Send register and value
*/


#include "msp430fg479.h"
#include <msp430.h>

#include "../functions/system_config.h"
#include "../functions/general_functions.h"
#include "../functions/SD16_A.h"

#include "IKKI_MAC.h"

#include "../config/SD16A_config.h"
#include "../config/SPI_config.h"
#include "../config/UART_config.h"
#include "../config/clk_config.h"



#include "../../../common/INTAN_config.h"


int state = 1;
uint8_t high_word = 0x00;
uint8_t low_word = 0xFF;
bool high_or_low = true;
uint16_t my_register = 0;
uint8_t packet_1 = 1;
uint8_t packet_2 = 2;
uint8_t packet_3 = 3;
uint8_t packet_4 = 4;
uint16_t pckt_count = 0;


#define UART_USAGE    false //True: UART, False: SPI
#define SD16_USAGE    false
#define TEST_CLK      false
#define SENSE_OR_STIM 2 //1:SENSE 2:STIM 

CLK_config_struct CLK_config;
SD16A_config_struct SD16A_configuration;
UART_config_struct UART_config;
SPI_config_struct SPI_config;
INTAN_config_struct INTAN_config;



int main(void) {
  general_setup(CLK_config);
  init_MSP();

  //************************** CLK configuration *****************************
  
  setup_CLK(CLK_config);

  //************************** LED configuration *****************************
  toggle_setup(); // Setup P4.6 for LED output
#if (TEST_CLK)
  configure_PINS_for_clk_debug();

#else

  #if (UART_USAGE)
    //************************** UART configuration *****************************
    setup_UART(UART_config);

  #else
    //************************** SPI configuration *****************************
    SPI_configuration(SPI_config);

  #endif

  #if (SENSE_OR_STIM==1)
    #if (SD16_USAGE)
      //************************** SD16 configuration *****************************
      setup_SD16A(SD16A_configuration);
      enable_interruption_SD16A(true);
      start_conversion(); // While it is started, working in continuous mode will
                          // sample the channel A until it is stopped
    #endif
      UCA0TXBUF = 0x45; // Transmit first character

      enable_interruptions(true);

      IE2 |= UCA0RXIE+UCA0TXIE; // Enabling UART interrupt
  #elif(SENSE_OR_STIM == 2)
    CS_setup();
    ON_CS_pin();
    stim_en_setup();

    pckt_count = 0;
    INTAN_config.step_sel = 1000;

    INTAN_config.target_voltage = 1.2;
    INTAN_config.CL_sel = 1;
    stim_en_OFF();
    // create_arrays(&INTAN_config);
    create_stim_SPI_arrays(&INTAN_config);
    while(1){
      if(pckt_count == INTAN_config.max_size){
            stim_en_ON();
      }else{
        if (state == 1){
          update_packets(pckt_count, &packet_1, &packet_2, &packet_3, &packet_4, INTAN_config);
          pckt_count++;
          state = 2;      
        }
        else if (state == 2){
          OFF_CS_pin();
          state = 1;
          while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
          UCA0TXBUF = packet_1;
          while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
          UCA0TXBUF = packet_2;
          while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
          UCA0TXBUF = packet_3;
          while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
          UCA0TXBUF = packet_4;
          while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
          ON_CS_pin(); 
        }
      }
    }
  #endif
  }
#endif







#if (SENSE_OR_STIM == 1 && TEST_CLK == false)

  //***************************************************************************** 
  //Interrupción de la UART
  //***************************************************************************** 
  #if (UART_USAGE == true)
      //TX interrupt handler

      #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
      #pragma vector=USCIAB0TX_VECTOR
      __interrupt void USCI_A0_Tx (void)
      #elif defined(__GNUC__)
      void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCI_A0_Tx (void)
      #else
      #error Compiler not supported!
      #endif
      {
          if (state == 1){
              UCA0TXBUF = SD16A_configuration.analog_input_ID[SD16A_configuration.analog_input_being_sampled];
              state = 2;
              SD16CCTL0 |= SD16IE;          // Enabling SD16 interrupt    
              IE2 &= ~(UCA0RXIE|UCA0TXIE);  // Disabling UART interrupt

          }
          if (state == 3) {
              if (high_or_low) {
                  UCA0TXBUF = high_word;    // Envía el byte alto
              } else {
                  UCA0TXBUF = low_word;     // Envía el byte bajo
                  state = 1;                // Solo aquí finalizamos la transmisión completa de los bytes
                  SD16CCTL0 &= ~(SD16IE);   // Disabling SD16 interrupt
                  IE2 |= UCA0RXIE+UCA0TXIE; // Enabling UART interrupt
              }
              high_or_low = !high_or_low;   // Alterna entre alto y bajo
          }
      }
  #else

    //*****************************************************************************
    // Interrupción del SPI
    //*****************************************************************************
    #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
    #pragma vector = USCIAB0RX_VECTOR
    __interrupt void USCIA0RX_ISR(void)
    #elif defined(__GNUC__)
    void __attribute__((interrupt(USCIAB0RX_VECTOR))) USCIA0RX_ISR(void)
    #else
    #error Compiler not supported!
    #endif
    {
      if (state == 1) {
        UCA0TXBUF = SD16A_configuration.analog_input_ID[SD16A_configuration.analog_input_being_sampled];
        state = 2;
        SD16CCTL0 |= SD16IE;         // Enabling SD16 interrupt
        IE2 &= ~(UCA0RXIE);          // Disabling SPI interrupt
      }
      if (state == 3) {
        if (high_or_low) {
          UCA0TXBUF = high_word;      // Envía el byte alto
          high_or_low = !high_or_low; // Alterna entre alto y bajo

        } else {
          UCA0TXBUF = low_word;       // Envía el byte bajo
          state = 1;                  // Solo aquí finalizamos la transmisión completa de los bytes
          SD16CCTL0 &= ~(SD16IE);     // Disabling SD16 interrupt
          IE2 |= UCA0RXIE;            // Enabling SPI interrupt
          high_or_low = !high_or_low; // Alterna entre alto y bajo
        }
      }
    }
  #endif




  //*****************************************************************************
  // Interrupción del SD16_A
  //*****************************************************************************
  #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
  #pragma vector = SD16A_VECTOR
  __interrupt void SD16ISR(void)
  #elif defined(__GNUC__)
  void __attribute__((interrupt(SD16A_VECTOR))) SD16ISR(void)
  #else
  #error Compiler not supported!
  #endif
  {
    switch (SD16IV) {
    case 2: // SD16MEM Overflow
      state = 5;
      break;
    case 4: // SD16MEM0 IFG
      if (state == 2) {
        my_register = SD16MEM0; // Save CH0 results (clears IFG)
        SD16A_configuration.analog_input_being_sampled++;
        if (SD16A_configuration.analog_input_being_sampled == SD16A_configuration.analog_input_count) {
          SD16A_configuration.analog_input_being_sampled = 0;
        }
        // select_analog_input(SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled]);
        // First, clean the bits so to not have more than one channel reading
        SD16INCTL0 &=  ~(SD16INCH_0 | SD16INCH_1 | SD16INCH_2 | SD16INCH_3 | SD16INCH_4);
        SD16INCTL0 |= SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled];

        high_word = (my_register >> 8) & 0xFFFF; // 8 bits superiores (0x1234)
        low_word = my_register & 0xFFFF;         // 8 bits inferiores (0x5678)
        state = 3;
        IE2 |= UCA0RXIE;                         // Enabling UART interrupt
        SD16CCTL0 &= ~(SD16IE);                  // Disabling SD16 interrupt
      } else {
        IE2 |= UCA0RXIE;                         // Enabling UART interrupt
        SD16CCTL0 &= ~(SD16IE);                  // Disabling SD16 interrupt
      }
      break;
    }
  }
#endif

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
#include "../functions/FLL.h"

#include "IKKI_MAC.h"

#include "../config/SD16A_config.h"
#include "../config/SPI_config.h"
#include "../config/UART_config.h"
#include "../config/clk_config.h"



#include "../../../common/INTAN_config.h"
#include "../../../common/register_macro.h"



int state = 1;
volatile uint8_t high_word = 0x00;
volatile uint8_t low_word = 0xFF;
bool high_or_low = true;
int i;
uint16_t my_register = 0;



#define UART_USAGE    false //True: UART, False: SPI
#define TEST_CLK      false
#define SENSE_OR_STIM 3 //1: SENSE interruptions 2: SENSE while 3:STIM 
#if (SENSE_OR_STIM != 3)
  #define ECG_or_EEG 1    //1: ECG experiment 2: EEG experiment 3: ECG + BAT 4: EEG + BAT 5: All channels
  #define SD16_USAGE true
#elif(SENSE_OR_STIM ==3)
  uint8_t num_stim = NUM_STIM_EXP;

  uint8_t packet_1 = 1;
  uint8_t packet_2 = 2;
  uint8_t packet_3 = 3;
  uint8_t packet_4 = 4;

#endif

CLK_config_struct CLK_config;
SD16A_config_struct SD16A_configuration;
UART_config_struct UART_config;
SPI_config_struct SPI_config;
INTAN_config_struct INTAN_config;

#if (SENSE_OR_STIM != 3)
void config_SD16A(SD16A_config_struct* SD16A_configuration){
  
      // -- Entrada analógica
      SD16A_configuration->analog_input_being_sampled = 0;
#if (ECG_or_EEG==1)
      SD16A_configuration->analog_input_count = 1;
      SD16A_configuration->analog_input[0] = ECG;  // 0: A0 - ECG
      SD16A_configuration->analog_input_ID[0] = 0x30; // 0: A0 - ECG

#elif (ECG_or_EEG==2)
      SD16A_configuration->analog_input_count = 3;

      SD16A_configuration->analog_input[0] = EEG3; // 1: A1 - EEG3
      SD16A_configuration->analog_input[1] = EEG2; // 2: A2 - EEG2
      SD16A_configuration->analog_input[2] = EEG1; // 3: A3 - EEG1

      SD16A_configuration->analog_input_ID[0] = 0x30; // 2: A2 - EEG2
      SD16A_configuration->analog_input_ID[1] = 0x31; // 1: A1 - EEG3
      SD16A_configuration->analog_input_ID[2] = 0x32; // 3: A3 - EEG1
#elif (ECG_or_EEG==3)
      SD16A_configuration->analog_input_count = 2;

      SD16A_configuration->analog_input[0] = ECG;  // 0: A0 - ECG
      SD16A_configuration->analog_input[1] = BATT; // 4: A4 - BATT

      SD16A_configuration->analog_input_ID[0] = 0x30; // 0: A0 - ECG
      SD16A_configuration->analog_input_ID[1] = 0x31; // 4: A4 - BATT

#elif (ECG_or_EEG==4)
      SD16A_configuration->analog_input_count = 4;

      SD16A_configuration->analog_input[0] = EEG3; // 1: A1 - EEG3
      SD16A_configuration->analog_input[1] = EEG2; // 2: A2 - EEG2
      SD16A_configuration->analog_input[2] = EEG1; // 3: A3 - EEG1
      SD16A_configuration->analog_input[3] = BATT; // 4: A4 - BATT

      SD16A_configuration->analog_input_ID[0] = 0x30; // 1: A1 - EEG3
      SD16A_configuration->analog_input_ID[1] = 0x31; // 2: A2 - EEG2
      SD16A_configuration->analog_input_ID[2] = 0x32; // 3: A3 - EEG1
      SD16A_configuration->analog_input_ID[3] = 0x33; // 4: A4 - BATT

#elif (ECG_or_EEG==5)
      SD16A_configuration->analog_input_count = 5;

      SD16A_configuration->analog_input[0] = ECG;  // 0: A0 - ECG
      SD16A_configuration->analog_input[1] = EEG3; // 2: A2 - EEG2
      SD16A_configuration->analog_input[2] = EEG2; // 1: A1 - EEG3
      SD16A_configuration->analog_input[3] = EEG1; // 3: A3 - EEG1
      SD16A_configuration->analog_input[4] = BATT; // 4: A4 - BATT

      SD16A_configuration->analog_input_ID[0] = 0x30; // 0: A0 - ECG
      SD16A_configuration->analog_input_ID[1] = 0x31; // 2: A2 - EEG2
      SD16A_configuration->analog_input_ID[2] = 0x32; // 1: A1 - EEG3
      SD16A_configuration->analog_input_ID[3] = 0x33; // 3: A3 - EEG1
      SD16A_configuration->analog_input_ID[4] = 0x34; // 4: A4 - BATT
#endif

      // -- Tensión de referencia
      SD16A_configuration->v_ref = 'I'; // I: Internal (1.2V), O: Off-chip, E: External
              // -- Reloj de referencia
      SD16A_configuration->clk_ref = 'M'; // M: MCLK, S: SMCLK, A: AC1LK, T: TACLK
                                        // -- Divisor de frecuencia de referencia
      SD16A_configuration->clk_div_1 = 1;
      SD16A_configuration->clk_div_2 = 1;
      // -- Método de lectura: Polling o Interrupciones
      SD16A_configuration->interruption_SD16A = true;
      // -- Over Sampling Ratio
      SD16A_configuration->OSR = 512; // 1, 32, 64, 128, 256, 512, 1024
      // -- Ganancia
      SD16A_configuration->gain = 1; // 1, 2, 4, 8, 16 or 32
      // -- Método de conversión
      SD16A_configuration->conv_mode = 'C'; // C: Continuous  S: Single
                                          // -- Tipo de datos
      SD16A_configuration->polarity = 'B';  // B : Bipolar, U : unipolar
      SD16A_configuration->sign = 'O';      // O : Offset, C : 2's complement

      SD16A_configuration->sampled = false;
}
#endif
void config_SPI(SPI_config_struct* SPI_config){
  SPI_config->Master_Slave = 'M';       //M: MSB, L: LSB
  SPI_config->inactive_state = 'H';     // clock polarity inactive high
  //data cHanged on the first UCLK edge and captured on the following edge
  //data cAptured on the first UCLK edge and changed on the following edge
  SPI_config->data_on_clock_edge = 'H'; // data cAptured on the first UCLK edge and changed on the following edge
  SPI_config->SPI_length = 8;
  SPI_config->first_Byte_sent = 'M';
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

void init_SPI_for_Intan(uint16_t clk_divisor) {
    // Poner en reset al USCI_A0
    UCA0CTL1 |= UCSWRST;

    // Limpiar todos los registros USCI_A0
    UCA0CTL0 = 0;        // Borra CPOL, CPHA, MSB/LSB, modo master/slave, síncrono
    UCA0CTL1 = 0;
    UCA0BR0 = 0;
    UCA0BR1 = 0;
    UCA0MCTL = 0;

    // Limpiar pines de selección
    P2SEL &= ~(BIT4 | BIT5);
    P3SEL &= ~BIT0;

    // Configuración de pines SPI
    P2SEL |= BIT4 | BIT5; // SIMO, SOMI
    P3SEL |= BIT0;         // CLK

    // Configuración USCI_A0 en SPI Master, MSB first, Mode 0
    UCA0CTL0 = UCMST   // Master
             | UCSYNC  // Síncrono
             | UCMSB;  // MSB first
    // CPOL=0, CPHA=0, por defecto

    // Fuente de clock: SMCLK
    UCA0CTL1 |= UCSSEL_2;

    // Divisor de reloj
    UCA0BR1 = clk_divisor / 256;
    UCA0BR0 = clk_divisor % 256;

    // Sin modulación
    UCA0MCTL = 0;

    // Quitar reset → activar USCI_A0
    UCA0CTL1 &= ~UCSWRST;
}




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

  #if (SENSE_OR_STIM==3)
      config_SPI_stim(&SPI_config);
      // config_SPI(&SPI_config);
      SPI_setup(&SPI_config);


      // init_SPI_for_Intan(2);
      INTAN_LED_setup(); // Setup P4.3 for LED output
      OFF_INTAN_LED();

      initialize_INTAN(&INTAN_config);
    
  #else
    #if (UART_USAGE)
      //************************** UART configuration *****************************
      setup_UART(UART_config);

    #else
      //************************** SPI configuration *****************************
      config_SPI(&SPI_config);
      SPI_setup(&SPI_config);

    #endif
  #endif

  #if (SENSE_OR_STIM==1)
    #if (SD16_USAGE)
      //************************** SD16 configuration *****************************
      config_SD16A(&SD16A_configuration);
      setup_SD16A(&SD16A_configuration);
      enable_interruption_SD16A(true);
      start_conversion(); // While it is started, working in continuous mode will
                          // sample the channel A until it is stopped
    #endif
      UCA0TXBUF = 0x45; // Transmit first character

      enable_interruptions(true);

      IE2 |= UCA0RXIE+UCA0TXIE; // Enabling UART interrupt
  
  #elif(SENSE_OR_STIM == 2)
      UCA0TXBUF = SD16A_configuration.analog_input_ID[SD16A_configuration.analog_input_being_sampled];

      //************************** SD16 configuration *****************************
      config_SD16A(&SD16A_configuration);
      setup_SD16A(&SD16A_configuration);
      start_conversion(); // While it is started, working in continuous mode will

      SD16CCTL0 &= ~(SD16IE);   // Disabling SD16 interrupt
      IE2 &= ~(UCA0RXIE|UCA0TXIE);  // Disabling UART interrupt
      while(1){
        if (state == 1){
          my_register = SD16MEM0; // Save CH0 results (clears IFG)

          // 1. select_analog_input(SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled]);
          // First, clean the bits so to not have more than one channel reading
          SD16INCTL0 &=  ~(SD16INCH_0 | SD16INCH_1 | SD16INCH_2 | SD16INCH_3 | SD16INCH_4);
          SD16INCTL0 |= SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled];
          #if (ECG_or_EEG != 1)
            // 2. Wait for stability
            __delay_cycles(1600);        // 12.5 µs @ 8 MHz

            // 3. Start conversion and discard
            SD16CCTL0 |= SD16SC;
            while (!(SD16CCTL0 & SD16IFG));
            volatile int dummy = SD16MEM0;   // discard
          #endif

          // 4. Do real lecture
          SD16CCTL0 |= SD16SC;
          while (!(SD16CCTL0 & SD16IFG));
          
          my_register = SD16MEM0; // Save CH0 results (clears IFG)

          // Update the channel to be read the next
          SD16A_configuration.analog_input_being_sampled++;
          if (SD16A_configuration.analog_input_being_sampled == SD16A_configuration.analog_input_count) {
            SD16A_configuration.analog_input_being_sampled = 0;
          }
          high_word = (my_register >> 8) & 0xFFFF; // 8 bits superiores (0x1234)
          low_word = my_register & 0xFFFF;         // 8 bits inferiores (0x5678)
          state = 3;

        } else if (state == 3) {
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = SD16A_configuration.analog_input_ID[SD16A_configuration.analog_input_being_sampled];
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = high_word;    // Envía el byte alto
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = low_word;     // Envía el byte bajo
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            state = 1;                // Solo aquí finalizamos la transmisión completa de los bytes
          }
        }
 


    #elif(SENSE_OR_STIM == 3)
      CS_setup();
      ON_CS_pin();
      stim_en_setup();
      button_init();

      INTAN_config.step_sel = 1000;

      INTAN_config.target_voltage = 1.2;
      INTAN_config.CL_sel = 1;
      stim_en_OFF();
      // create_example_SPI_arrays(&INTAN_config);
      // create_stim_SPI_arrays(&INTAN_config);

      // check_intan_SPI_array(&INTAN_config);
      // clear_command(&INTAN_config);


      // while(pckt_count != INTAN_config.max_size){       
      //   if (state == 1){
      //     update_packets(pckt_count, &packet_1, &packet_2, &packet_3, &packet_4, INTAN_config);
      //     pckt_count++;
      //     state = 2;      
      //   }
      //   else if (state == 2){
      //     OFF_CS_pin();
      //     state = 1;
      //     while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
      //     UCA0TXBUF = packet_1;
      //     while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
      //     UCA0TXBUF = packet_2;
      //     while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
      //     UCA0TXBUF = packet_3;
      //     while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
      //     UCA0TXBUF = packet_4;
      //     while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
      //     ON_CS_pin();           
      //   }
      // }
      //   bool next_stim =true;

      // while(1){
      //   next_stim = button_pressed();
      //   if(next_stim == true){
      //     ON_pin();
      //     num_stim = NUM_STIM_EXP;
      //     while(num_stim > 0){
      //           // stim_en_ON();
      //           // wait_30_seconds();
      //           // stim_en_OFF();
      //           // wait_5_mins();
                
      //           stim_en_ON();
      //           wait_3_seconds();
      //           stim_en_OFF();
      //           wait_5_seconds();
                
      //           num_stim--;
      //     }
      //     OFF_pin();
      //   }
      // }
      


      while(1){
        bool next_stim = button_pressed();
        if(next_stim == true){
          check_intan_SPI_array(&INTAN_config);

          // read_command(&INTAN_config, STIM_ENABLE_A_reg);
          // read_command(&INTAN_config, STIM_ENABLE_B_reg);

          write_command(&INTAN_config, STIM_ENABLE_A_reg,0x0000);
          write_command(&INTAN_config, STIM_ENABLE_B_reg,0x0000);

          clear_command(&INTAN_config);

          // read_command(&INTAN_config, STIM_ENABLE_A_reg);
          // read_command(&INTAN_config, STIM_ENABLE_B_reg);

          // write_command(&INTAN_config, 0x00,0x0000);
          // read_command(&INTAN_config, 0x00);

          // fc_high(&INTAN_config, 'k', 7.5); //TODO crear una variable para K y para 7.5 y que se pueda configurar al inicio del todo
          // read_command(&INTAN_config, ADC_HIGH_FREQ_4);
          // read_command(&INTAN_config, ADC_HIGH_FREQ_5);
          // fc_low_A(&INTAN_config, 5.0);
          // fc_low_B(&INTAN_config, 1000);
          // read_command(&INTAN_config, ADC_LOW_FREQ_A);
          // read_command(&INTAN_config, ADC_LOW_FREQ_B);

          // enable_M_flag(&INTAN_config);
          // enable_U_flag(&INTAN_config);
          // enable_H_flag(&INTAN_config);
          // enable_D_flag(&INTAN_config);

          // convert_channel(&INTAN_config, 0);
          // convert_channel(&INTAN_config, 14);
          // convert_N_channels(&INTAN_config, 8);

          

          

          state = 1;
          send_SPI_commands(state, &INTAN_config, &packet_1, &packet_2, &packet_3, &packet_4);
          while(next_stim == true){
            next_stim = button_pressed();
          }
        }
      }
    #endif
  #endif
}




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
      if (state == 1) {
        UCA0TXBUF = SD16A_configuration.analog_input_ID[SD16A_configuration.analog_input_being_sampled];
        state = 2;
        SD16CCTL0 |= SD16IE;         // Enabling SD16 interrupt
        IE2 &= ~(UCA0RXIE+UCA0TXIE);          // Disabling SPI interrupt
      }else if (state == 3) {
        if (high_or_low) {
          UCA0TXBUF = high_word;      // Envía el byte alto
          high_or_low = !high_or_low; // Alterna entre alto y bajo

        } else {
          UCA0TXBUF = low_word;       // Envía el byte bajo
          state = 1;                  // Solo aquí finalizamos la transmisión completa de los bytes
          SD16CCTL0 &= ~(SD16IE);     // Disabling SD16 interrupt
          IE2 |= UCA0RXIE+UCA0TXIE;   // Enabling SPI interrupt

          high_or_low = !high_or_low; // Alterna entre alto y bajo
        }
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
        IE2 &= ~(UCA0RXIE+UCA0TXIE);          // Disabling SPI interrupt

      }else if (state == 3) {
        if (high_or_low) {
          UCA0TXBUF = high_word;      // Envía el byte alto
          high_or_low = !high_or_low; // Alterna entre alto y bajo

        } else {
          UCA0TXBUF = low_word;       // Envía el byte bajo
          state = 1;                  // Solo aquí finalizamos la transmisión completa de los bytes
          SD16CCTL0 &= ~(SD16IE);     // Disabling SD16 interrupt
          IE2 |= UCA0RXIE+UCA0TXIE;   // Enabling SPI interrupt

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
       
        // 1. select_analog_input(SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled]);
        // First, clean the bits so to not have more than one channel reading
        SD16INCTL0 &=  ~(SD16INCH_0 | SD16INCH_1 | SD16INCH_2 | SD16INCH_3 | SD16INCH_4);
        SD16INCTL0 |= SD16A_configuration.analog_input[SD16A_configuration.analog_input_being_sampled];
        #if (ECG_or_EEG != 1)
        // 2. Wait for stability
        __delay_cycles(2000);        // 12.5 µs @ 8 MHz

        // 3. Start conversion and discard
        SD16CCTL0 |= SD16SC;
        while (!(SD16CCTL0 & SD16IFG));
        volatile int dummy = SD16MEM0;   // discard
        #endif

        // 4. Do real lecture
        SD16CCTL0 |= SD16SC;
        while (!(SD16CCTL0 & SD16IFG));
        
        my_register = SD16MEM0; // Save CH0 results (clears IFG)

        // Update the channel to be read the next
        SD16A_configuration.analog_input_being_sampled++;
        if (SD16A_configuration.analog_input_being_sampled == SD16A_configuration.analog_input_count) {
          SD16A_configuration.analog_input_being_sampled = 0;
        }
        high_word = (my_register >> 8) & 0xFFFF; // 8 bits superiores (0x1234)
        low_word = my_register & 0xFFFF;         // 8 bits inferiores (0x5678)
        state = 3;
        IE2 |= UCA0RXIE+UCA0TXIE;                // Enabling SPI interrupt
        SD16CCTL0 &= ~(SD16IE);                  // Disabling SD16 interrupt
      } else {
        IE2 |= UCA0RXIE+UCA0TXIE;                // Enabling SPI interrupt
        SD16CCTL0 &= ~(SD16IE);                  // Disabling SD16 interrupt
      }
      break;
    }
  }
#endif

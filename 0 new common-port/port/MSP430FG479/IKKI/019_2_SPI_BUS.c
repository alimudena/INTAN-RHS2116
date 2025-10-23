//                   MSP430x47x
//                 -----------------
//             /|\|              XIN|-
//              | |                 |  32kHz xtal
//              --|  RST        XOUT|-
//                |  12         P4.6|--> LED
//                |                 |
//                |  --- INTAN ---  |            
//                |                 |
//                |  44         P3.0|-> SCLK B0  
//                |  43         P2.5|<- MISO B0  
//                |  42         P2.4|-> MOSI B0  
//                |  15         P4.3|--> LED for INTAN bug 
//                |  14         P4.4|--> CS for INTAN
//                |  13         P4.5|--> stim_en for INTAN
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
//                |  --- ESP32 ---  |                                           ESP32            
//                |                 |
//                |  18         P4.0|--> LED ESP32 communications indicator  
//                |  16         P4.2|-> CS for ESP32 -------------------------> - GPIO 15: A3 CS PARAM   
//                |  11         P4.7|-> CS for ESP32 ECG ---------------------> - GPIO 2: RX CS ECG                              - por si fuese necesaro habilitar otro bus SPI hacia el slave
//                |  41         P3.0|-> SCLK A0 ------------------------------> - GPIO 16: A2 SCLK PARAM 
//                |  75         P2.5|<- MISO A0 ------------------------------> - GPIO 17: A1 MISO PARAM 
//                |  76         P2.4|-> MOSI A0 ------------------------------> - GPIO 18: A0 MOSI PARAM 
//                |  54         P1.4|-> NEW_PARAM  ---------------------------> - GPIO 11: D11  
//                |  57         P1.1|<- ACK_PARAM  ---------------------------> - GPIO 12: D12
//                |  51         P1.5|-> ESP32 connected ----------------------> - GPIO 13: D13
//                |                 |-----------------------------------------> - GND
//                |                 |
//                |  ---  SPI  ---  |
//                |      USCIA0     |
//                |  76         P2.4|-> MOSI - USCIA0 
//                |  75         P2.5|<- MISO - USCIA0
//                |  41         P3.0|-> SCLK - USCIA0
//                |                 |
//                |  ---  SPI  ---  |
//                |      USCIB0     |
//                |  42         P2.4|-> MOSI - USCIB0 
//                |  43         P2.5|<- MISO - USCIB0
//                |  44         P3.0|-> SCLK - USCIB0
//                |                 |
//                |  ---  SD16 ---  |
//                |             P6.0|<------- A0+: ECG positive input --> 67
//                |             P6.1|<------- A0-: ECG negative input --> 66
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



#include <msp430.h>
#include <stdbool.h> 
#include <stdint.h>  

#include "IKKI_MAC.h"

#include "../config/SD16A_config.h"
#include "../config/SPI_config.h"
#include "../config/UART_config.h"
#include "../config/clk_config.h"
#include "../config/TIMER_config.h"

#include "../functions/system_config.h"
#include "../functions/general_functions.h"
#include "../functions/SD16_A.h"
#include "../functions/FLL.h"
#include "../functions/TIMER.h"


#include "../../../common/INTAN_config.h"
#include "../../../common/INTAN_functions.h"
#include "../../../common/register_macro.h"

volatile uint8_t high_word = 0x00;
volatile uint8_t low_word = 0xFF;
bool high_or_low = true;
int i;
uint16_t my_register = 0;

uint32_t timing_counter_during_stimulation = 0;
uint32_t timing_counter_ON_OFF = 0;
bool ON_OFF_stimulation = true;
uint8_t number_of_stimulations_done = 10;

uint8_t resting_time_minutes = 0;
uint8_t stimulation_time_seconds = 0;
uint8_t high_byte_stimulation_on_time_micro = 0;
uint8_t low_byte_stimulation_on_time_micro = 0;
uint16_t stimulation_on_time_micro = 0;
uint8_t stimulation_off_time_milis = 0;

uint32_t divider_value = 0x028F;

uint8_t RX;


bool new_parameters;
bool esp32_connected;
bool sent = false;

CLK_config_struct CLK_config;
SD16A_config_struct SD16A_configuration;
TIMER_config_struct TIMER_config;
INTAN_config_struct INTAN_config;
SPI_config_struct SPI_config;
SPI_B_config_struct SPI_B_config;


void config_SPI_A0_stim(SPI_config_struct* SPI_config){
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

void config_SPI_B0_stim(SPI_B_config_struct* SPI_B_config){
  SPI_B_config->Master_Slave = 'M';       //M: Master, S: Slave
  SPI_B_config->inactive_state = 'L';     // clock polarity inactive low
  //data cHanged on the first UCLK edge and captured on the following edge
  //data cAptured on the first UCLK edge and changed on the following edge
  SPI_B_config->data_on_clock_edge = 'A'; 
  SPI_B_config->SPI_length = 8;
  SPI_B_config->first_Byte_sent = 'M'; //M: MSB, L: LSB
    /*clk_ref:
      U --> UCLK
      A --> ACLK
      S --> SMCLK
  */
  SPI_B_config->clk_ref_SPI = 'S';
  SPI_B_config->clk_div = 2;
  SPI_B_config->enable_USCI_interr_rx = false;
  SPI_B_config->enable_USCI_interr_tx = false;
}


void config_SD16A(SD16A_config_struct* SD16A_configuration){
  
      // -- Entrada analógica
      SD16A_configuration->analog_input_being_sampled = 0;
      SD16A_configuration->analog_input_count = 1;
      SD16A_configuration->analog_input[0] = ECG;  // 0: A0 - ECG
      SD16A_configuration->analog_input_ID[0] = 0x30; // 0: A0 - ECG

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



int main(void)
{
  general_setup(CLK_config);
  init_MSP();
  enable_interruptions(true);


  //************************** CLK configuration *****************************
  setup_CLK(CLK_config);

  //************************** LED configuration *****************************
  toggle_setup();

  
  //************************** SPI comms configuration *****************************
  config_SPI_A0_stim(&SPI_config);
  SPI_setup(&SPI_config);

  config_SPI_B0_stim(&SPI_B_config);
  SPI_B_setup(&SPI_B_config);

  //************************** INTAN programming setup ************************** 
  INTAN_LED_setup(); // Setup P4.3 for LED output
  OFF_INTAN_LED();

  CS_INTAN_setup();
  ON_CS_INTAN_pin();
  // stim_en_setup();
  // stim_en_OFF();

  //************************** External button setup  ************************** 
  button_init();
  bool next_stim = button_pressed(); 

  //************************** ESP32 communication setup ************************** 
  CS_ESP_PARAM_setup();
  ON_CS_ESP_PARAM_pin();

  CS_ESP_ECG_setup();
  ON_CS_ESP_ECG_pin();

  ACK_param_setup();
  OFF_ACK_param();
  new_param_setup();
  ESP32_connected_setup();
  ESP32_LED_setup();

  //************************** SD16 configuration *****************************
  config_SD16A(&SD16A_configuration);
  setup_SD16A(&SD16A_configuration);
  start_conversion(); // While it is started, working in continuous mode will

  SD16CCTL0 &= ~(SD16IE);   // Disabling SD16 interrupt
  IE2 &= ~(UCA0RXIE|UCA0TXIE);  // Disabling UART interrupt
  
  //************************** TIMER setup ************************** 
  source_clock_select_TIMER('S');
  operating_mode_TIMER('U');

                                                                              TACCR0 = divider_value;

  //************************** INTAN setup ************************** 

  stim_en_OFF();
  initialize_INTAN(&INTAN_config);

  /* 
    STIMULATION DISABLE AND MINIMUM POWER DISIPATION
  */
  INTAN_config.ADC_sampling_rate = 480;
  /*
    DSP FOR HIGH PASS FILTER REMOVAL
  */
  INTAN_config.DSP_cutoff_freq = 4.665;
  INTAN_config.number_channels_to_convert = 8;

  /*
    ELECTRODE IMPEDANCE TEST
  */
  INTAN_config.zcheck_select = 0;
  INTAN_config.zcheck_load = 1;
  INTAN_config.zcheck_scale = 0;
  INTAN_config.zcheck_en = 0;
  INTAN_config.zcheck_DAC_value = 128;

  /*
    AMPLIFIER BANDWIDTH
  */

  INTAN_config.fh_magnitude = 7.5;
  INTAN_config.fh_unit = 'k';
  INTAN_config.fc_low_A = 5;
  INTAN_config.fc_low_B = 1000;

  /*
    CONSTANT CURRENT STIMULATOR
  */
  INTAN_config.step_DAC = 5000; // uA
  INTAN_config.negative_current_magnitude[0] = 100;
  INTAN_config.negative_current_trim[0] = 0x80;
  INTAN_config.positive_current_magnitude[0] = 100;
  INTAN_config.positive_current_trim[0] = 0x80;


  /*
    CURRENT LIMITED CHARGE RECOVERY CIRCUIT
  */             
  
  INTAN_config.voltage_recovery = 0;
  INTAN_config.current_recovery = 1;


  call_configuration_functions(&INTAN_config);

  /*
    STIMULATION PARAMETERS
  */
  INTAN_config.stimulation_time = 36641;
  // INTAN_config.stimulation_time = 366412;
  // INTAN_config.resting_time = 3664122;
  INTAN_config.resting_time = 36641;
  INTAN_config.stimulation_on_time = 6;
  INTAN_config.stimulation_off_time = 604;    

  INTAN_config.number_of_stimulations = 2;

  /*State machine for stimulation parameters*/
  /*

          state 0: positive stimulation
          state 1: neutral stimulation
          state 3: negative stimulation
          state 4: neutral stimulation

  */

  /*State machine for ESP32 communications*/




  // //************************** other parameters ************************** 


    typedef enum {ENVIO_ECG, RX_PARAMS_ESP32, TX_PARAMS_INTAN, NEW_PARAM_OFF_WAIT} Estados;
    Estados general_state = ENVIO_ECG;

    bool new_param = false;

    bool stimulation_disabled = false; 
    uint8_t state_INTAN = 0;
    bool pos_neg = true;
    uint32_t T_on = INTAN_config.stimulation_on_time;
    uint32_t T_stim = INTAN_config.stimulation_on_time;

    while(1){
      switch (general_state) {
        case ENVIO_ECG: // Estado 1: Muestreo del ECG y envío a través de SPI al ESP32

          new_param = new_param_read();
          if(new_param){
            general_state = RX_PARAMS_ESP32;
            break;
          }else{

            /*
                    SEND TO ESP32 THE READ VALUES FROM ECG
            */

            // It is always sending data to the ESP32, if the timing pin is enabled then, the incorporated led will switch
            OFF_CS_ESP_PARAM_pin();
            while (!(SD16CCTL0 & SD16IFG));
            my_register = SD16MEM0; // Save CH0 results (clears IFG)
            high_word = (my_register >> 8) & 0xFFFF; // 8 bits superiores (0x1234)
            low_word = my_register & 0xFFFF;         // 8 bits inferiores (0x5678)
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = 0x30;
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            RX = UCA0RXBUF;
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = high_word;
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            RX = UCA0RXBUF;
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = low_word;
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            RX = UCA0RXBUF;    
            ON_CS_ESP_PARAM_pin();
            






            next_stim = button_pressed();
            if(next_stim){  
              number_of_stimulations_done = 0;
            }

            /*
              SEND DATA TO INTAN BECAUSE OF STATE CHANGE caused by a timer            
            */

            if(TACTL & TAIFG){ // POLLING AL BIT DE INTERRUPCION DEL TIMER
              TACTL &= ~TAIFG; // limpiar el flag
                                                                                                                TACCR0 = 0x028F;
              if(ON_OFF_stimulation){
                T_on = INTAN_config.stimulation_time;
              }else{
                T_on = INTAN_config.resting_time;
              }

              if(pos_neg){
                T_stim = INTAN_config.stimulation_on_time;
              }else{
                T_stim = INTAN_config.stimulation_off_time;          
              }

              if(number_of_stimulations_done < INTAN_config.number_of_stimulations){ // si el número de estimulaciones hechas es menor al que se quiere hacer
                if(timing_counter_ON_OFF >= T_on){  // si se ha llegado al contador de tiempo encendido/apagado
                  timing_counter_ON_OFF = 0;
                  ON_OFF_stimulation = !ON_OFF_stimulation;
                  if(ON_OFF_stimulation){ // si se ha estimulado se actualiza el contador
                    number_of_stimulations_done++;
                  }else{ // si se ha apagado la estimulación se deshabilita
                    if(!stimulation_disabled){ 
                      OFF_pin();
                      OFF_INTAN(&INTAN_config);
                      stimulation_disabled = true;
                    }
                  }
                }
                timing_counter_ON_OFF++;  
              


                if(ON_OFF_stimulation){ // si la estimulación está habilitada
                  ON_pin();
                  stimulation_disabled = false;
                  if(timing_counter_during_stimulation >= T_stim){ // y se ha contado el periodo de la señal positiva / negativa / neutra
                    timing_counter_during_stimulation = 0; // se reinicia el contador y se cambia de estado
                    switch (state_INTAN) {
                          case 0:
                            state_INTAN = 1;
                            INTAN_config.stimulation_on[0] = 1;
                            INTAN_config.stimulation_pol[0] = 'P';
                            ON_INTAN(&INTAN_config);
                            pos_neg = true;
                            break;
                          case 1:
                            state_INTAN = 2;
                            OFF_INTAN(&INTAN_config);
                            pos_neg = false;
                            break;  
                          case 2:
                            state_INTAN = 3;
                            INTAN_config.stimulation_pol[0] = 'N';
                            ON_INTAN(&INTAN_config);
                            pos_neg = true;
                            break;  
                          case 3:
                            state_INTAN = 0;
                            OFF_INTAN(&INTAN_config);
                            pos_neg = false;
                            break;  
                          default:
                            perror("Error: not corret state_INTAN.");
                            break;  
                      }
                  }
                  timing_counter_during_stimulation++;
                }
              }else{
                next_stim = button_pressed();
                OFF_pin();
              }
            }

          }







          break;
          

        case RX_PARAMS_ESP32: // Estado 2: Recepción de parámetros de estimulación nuevos a través del ESP32
          ON_ACK_param();
          __delay_cycles(8000);


          ON_ESP32_LED(); // se mantiene el led de recepción encendido como indicador para el usuario
          OFF_CS_ESP_PARAM_pin();
          UCA0TXBUF = 0x10;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          INTAN_config.number_of_stimulations = UCA0RXBUF;
          UCA0TXBUF = 0x20;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          resting_time_minutes = UCA0RXBUF;
          UCA0TXBUF = 0x30;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          stimulation_time_seconds = UCA0RXBUF;
          UCA0TXBUF = 0x40;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          high_byte_stimulation_on_time_micro = UCA0RXBUF;
          UCA0TXBUF = 0x50;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          low_byte_stimulation_on_time_micro = UCA0RXBUF;
          UCA0TXBUF = 0x60;
          while (!(IFG2 & UCA0RXIFG));              // USART1 TX buffer ready?
          stimulation_off_time_milis = UCA0RXBUF;
          ON_CS_ESP_PARAM_pin();
          stimulation_on_time_micro = ((uint16_t)high_byte_stimulation_on_time_micro << 8) | low_byte_stimulation_on_time_micro;

          INTAN_config.resting_time = resting_time_minutes*60*FREQ_MASTER/divider_value;
          INTAN_config.stimulation_time = stimulation_time_seconds*FREQ_MASTER/divider_value;
          INTAN_config.stimulation_on_time = stimulation_on_time_micro*FREQ_MASTER/(1000000*divider_value);
          INTAN_config.stimulation_off_time = stimulation_off_time_milis*FREQ_MASTER/(1000*divider_value);

          general_state = NEW_PARAM_OFF_WAIT;
          break;
        
        case NEW_PARAM_OFF_WAIT:
          new_param = new_param_read();
          if(!new_param){
            general_state = TX_PARAMS_INTAN;
            break;
          }
          
          break;
        case TX_PARAMS_INTAN: // Estado 3: actualización de parámetros de estimulación del INTAN

          general_state = ENVIO_ECG;
          number_of_stimulations_done = 100;
          OFF_ACK_param();
          break;
        default:
          break;

      }

      // Comprobar los flags del timer y actualizar parámetros del INTAN si es necesario

      


    
    }
}

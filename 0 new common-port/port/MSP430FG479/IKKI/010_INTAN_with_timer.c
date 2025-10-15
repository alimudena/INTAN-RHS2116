//                   MSP430x47x
//                 -----------------
//             /|\|              XIN|-
//              | |                 |  32kHz xtal
//              --|  RST        XOUT|-
//                |  12         P4.6|--> LED
//                |                 |
//                |  --- INTAN ---  |            
//                |                 |
//                |  14         P4.4|--> CS for INTAN
//                |  13         P4.5|--> stim_en for INTAN
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
//                |  --- ESP32 ---  |                                           ESP32            
//                |                 |
//                |  16         P4.2|-> CS for ESP32 -------------------------> - GPIO 15: A5
//                |  76         P2.4|-> Data Out (UCA0SIMO) ------------------> - GPIO 13: D13
//                |  75         P2.5|<- Data In (UCA0SOMI) -------------------> - GPIO 14: A4
//                |  41         P3.0|-> Serial Clock Out (UCA0CLK) -----------> - GPIO 12: D12
//                |  57         P1.1|-> Timing alert ESP32 change ------------> - GPIO 2:  RX
//                |  54         P1.4|-> Stimulation enable ESP32 -------------> - GPIO 1:  TX
//                |  51         P1.5|-> New parameters available ESP32 -------> - GPIO 8:  A5
//                |                 |-----------------------------------------> - GND
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


#include <msp430.h>
#include <stdbool.h> 
#include <stdint.h>  

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


uint32_t timing_counter_during_stimulation = 0;
uint32_t timing_counter_ON_OFF = 0;
bool ON_OFF_stimulation = true;
uint8_t number_of_stimulations_done = 0;

uint8_t state = 0;

CLK_config_struct CLK_config;
TIMER_config_struct TIMER_config;
INTAN_config_struct INTAN_config;
SPI_config_struct SPI_config;


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
  bool next_stim = button_pressed(); 

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
  INTAN_config.stimulation_time = 366412;
  INTAN_config.resting_time = 3664122;
  INTAN_config.stimulation_on_time = 6;
  INTAN_config.stimulation_off_time = 604;    

  INTAN_config.number_of_stimulations = 5;

  /*State machine for stimulation parameters*/
  /*

          state 0: positive stimulation
          state 1: neutral stimulation
          state 3: negative stimulation
          state 4: neutral stimulation

  */


  /*
    TIMER SETUP 
  */
  source_clock_select_TIMER('S');
  operating_mode_TIMER('U');

  TACCR0 = 0x028F;


  //************************** other parameters ************************** 
  volatile uint8_t RX;
  volatile uint8_t TX = 0;
  bool stimulation_disabled = false; 
  uint8_t state = 0;
  bool pos_neg = true;
  uint32_t T_on = INTAN_config.stimulation_on_time;
  uint32_t T_stim = INTAN_config.stimulation_on_time;


  while(1){

    while(!next_stim){
      next_stim = button_pressed();
        number_of_stimulations_done = 0;
    }
    // next_stim = button_pressed();
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
          stimulation_disabled = false;
          if(timing_counter_during_stimulation >= T_stim){ // y se ha contado el periodo de la señal positiva / negativa / neutra
            timing_counter_during_stimulation = 0; // se reinicia el contador y se cambia de estado
            switch (state) {
                  case 0:
                    state = 1;
                    INTAN_config.stimulation_on[0] = 1;
                    INTAN_config.stimulation_pol[0] = 'P';
                    ON_INTAN(&INTAN_config);
                    toggle_pin();
                    pos_neg = true;
                    break;
                  case 1:
                    state = 2;
                    OFF_INTAN(&INTAN_config);
                    toggle_pin();
                    pos_neg = false;
                    break;  
                  case 2:
                    state = 3;
                    INTAN_config.stimulation_pol[0] = 'N';
                    ON_INTAN(&INTAN_config);
                    toggle_pin();
                    pos_neg = true;
                    break;  
                  case 3:
                    state = 0;
                    OFF_INTAN(&INTAN_config);
                    toggle_pin();
                    pos_neg = false;
                    break;  
                  default:
                    perror("Error: not corret state.");
                    break;  
              }
          }
          timing_counter_during_stimulation++;
        }
      }else{
        next_stim = button_pressed();
        number_of_stimulations_done = 0;
      }
    }
  }
}


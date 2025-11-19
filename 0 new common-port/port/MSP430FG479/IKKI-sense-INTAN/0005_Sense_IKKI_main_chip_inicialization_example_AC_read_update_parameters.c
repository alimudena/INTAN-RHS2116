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

uint32_t timer_flag_on_off = 0;
uint32_t timer_flag_stim_rest = 0;
bool INTAN_programmed = false;
uint8_t number_of_stimulations_done = 10;

uint8_t high_ADC_sampling_rate;
uint8_t low_ADC_sampling_rate;
uint8_t high_ADC_sampling_rate_2;
uint8_t low_ADC_sampling_rate_2;
uint8_t high_ADC_sampling_rate_3;
uint8_t high_high_byte_DSP_cutoff_frequency;
uint8_t high_byte_DSP_cutoff_frequency;
uint8_t low_byte_DSP_cutoff_frequency;
uint8_t low_low_byte_DSP_cutoff_frequency;
uint8_t high_high_byte_fc_high_magnitude;
uint8_t high_byte_fc_high_magnitude;
uint8_t low_byte_fc_high_magnitude;
uint8_t low_low_byte_fc_high_magnitude;
uint8_t high_high_byte_fc_low_A;
uint8_t high_byte_fc_low_A;
uint8_t low_byte_fc_low_A;
uint8_t low_low_byte_fc_low_A;
uint8_t high_high_byte_fc_low_B;
uint8_t high_byte_fc_low_B;
uint8_t low_byte_fc_low_B;
uint8_t low_low_byte_fc_low_B;
uint8_t amplifier_cutoff;
uint8_t fc_high_unit;
char amplifier_cutoff_frequency_A_B;

typedef union {
    float value;
    uint8_t bytes[4];
} float_bytes_union;


uint32_t divider_value = 0x029A;

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


uint8_t channel = 0;


//************************** other parameters ************************** 

bool new_param = false;

uint32_t stim_on_off_time;
uint32_t stim_rest_time;
bool next_stim;

//************************** State machines ************************** 

typedef enum {ENVIO_INTAN, RX_PARAMS_ESP32, TX_PARAMS_INTAN, NEW_PARAM_OFF_WAIT} Estados;
Estados general_state = ENVIO_INTAN;


typedef enum {STIM, REST} states_stimulation;
states_stimulation state_stimulation = REST;



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

void configure_INTAN(INTAN_config_struct* INTAN_config){

  /* 
    STIMULATION DISABLE AND MINIMUM POWER DISIPATION
  */
  INTAN_config->ADC_sampling_rate = 13;
  /*
    DSP FOR HIGH PASS FILTER REMOVAL
  */
  INTAN_config->DSP_cutoff_freq = 4.665;
  INTAN_config->number_channels_to_convert = 1;

  /*
    ELECTRODE IMPEDANCE TEST
  */
  INTAN_config->zcheck_select = 0;
  INTAN_config->zcheck_load = 0;
  INTAN_config->zcheck_scale = 0;
  INTAN_config->zcheck_en = 0;
  INTAN_config->zcheck_DAC_power = 1;
  INTAN_config->zcheck_DAC_value = 128;

  /*
    AMPLIFIER BANDWIDTH
  */
   
  INTAN_config->fh_magnitude = 7.5;
  INTAN_config->fh_unit = 'k';
  INTAN_config->fc_low_A = 5;
  INTAN_config->fc_low_B = 1000.0;

  INTAN_config->amplifier_cutoff_frequency_A_B[channel] = 'A';

  /*
    CONSTANT CURRENT STIMULATOR
  */
  INTAN_config->step_DAC = 5000; // uA
  INTAN_config->negative_current_magnitude[0] = 100;
  INTAN_config->negative_current_trim[0] = 0x80;
  INTAN_config->positive_current_magnitude[0] = 100;
  INTAN_config->positive_current_trim[0] = 0x80;

  INTAN_config->MASTER_FREQ = 4200000;


  /*
    CURRENT LIMITED CHARGE RECOVERY CIRCUIT
  */             
  
  INTAN_config->voltage_recovery = 0;
  INTAN_config->current_recovery = 1;

  /*
    STIMULATION PARAMETERS
  */
  INTAN_config->stimulation_time = 0;
  // INTAN_config->stimulation_time = 366412;
  // INTAN_config->resting_time = 3664122;
  INTAN_config->resting_time = 0;
  INTAN_config->stimulation_on_time = 0;
  INTAN_config->stimulation_off_time = 0;    

  INTAN_config->number_of_stimulations = 2;
}

void config_CLK(CLK_config_struct* CLK_config){
  // Modo de operación
  /*    mode:
          - A: Active
          - L: Low Power
              - 0: LPM0
              - 1: LPM1
              - 2: LPM2
              - 3: LPM3
  */
  CLK_config->operating_mode = 'A';
  // Oscilador LFXT1
  /*
      L: Low Frequency Mode --> f auxiliar de 323kHz conectado
      H: High Frequency Mode
  */
  CLK_config->LFXT1_wk_mode = 'L';
  // Configura la capacidad interna del LFXT1
  /*0  --> XIN Cap = XOUT Cap = 0pf */
  /*10 --> XIN Cap = XOUT Cap = 10pf */
  /*14 --> XIN Cap = XOUT Cap = 14pf */
  /*18 --> XIN Cap = XOUT Cap = 18pf */
  CLK_config->LFXT1_int_cap = 0;
  // DCO
  // Rango de frecuencia de trabajo del DCO:
  /*2 --> fDCOCLK =   1.4-12MHz*/
  /*3 --> fDCOCLK =   2.2-17Mhz*/
  /*4 --> fDCOCLK =   3.2-25Mhz*/ //-> 8 MHz
  /*8 --> fDCOCLK =     5-40Mhz*/
  CLK_config->DCO_range = 4;
  // Values for setting the frequency of the DCO+
  // DCO+ set so freq= xtal x D x N_MCLK+1
  // XTAL --> 32767Hz
  CLK_config->DCOPLUS_on = true;                // If D factor is wanted to be applied then -> True
  CLK_config->D_val = 2;    // Max 8
  CLK_config->N_MCLK = 127; // Max 127
  // CLK_config->N_MCLK = 95; // Max 127
  // MCLK
  // Reference selection for MCLK
  CLK_config->ref_MCLK = 'D'; //  D: DCO, X: XT2, A: LFXT1
  // ACLK
  // ACLK division for configuring ACLK/N
  CLK_config->divider_ACLK = 1; // 1, 2, 4, 8
  // SMCLK
  //  Reference for SMCLK
  CLK_config->ref_SMCLK = 'D'; // D: DCO, X: XT2, N: OFF
  // LFXT2
  // Second oscillator ON OFF
  CLK_config->LFXT2_osc_on = false;
}

int main(void)
{


  /* COMMON CONFIGURATION */
  
  general_setup(CLK_config);
  init_MSP();
  enable_interruptions(true);


  //************************** CLK configuration *****************************
  config_CLK(&CLK_config);
  run_functions_setup_CLK(CLK_config);

  //************************** LED configuration *****************************
  toggle_setup();

  //************************** SPI comms configuration INTAN *****************************
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
  next_stim = button_pressed(); 


  //************************** TIMER setup ************************** 
  source_clock_select_TIMER('S');
  operating_mode_TIMER('U');
                                                                              TACCR0 = divider_value;


  ESP32_connected_setup();
  esp32_connected =  ESP32_connected();
  ESP32_LED_setup();

  if(esp32_connected){
    /* NOT LOW POWER CONFIGURATION */
    //The ESP32 is connected so turn on its led
    OFF_ESP32_LED();
    esp32_connected =  ESP32_connected();
  }

  ON_ESP32_LED();
  //************************** SPI comms configuration ESP32 *****************************
  config_SPI_A0_stim(&SPI_config);
  SPI_setup(&SPI_config);

  //************************** ESP32 communication setup ************************** 
  CS_ESP_PARAM_setup();
  ON_CS_ESP_PARAM_pin();

  CS_ESP_ECG_setup();
  ON_CS_ESP_ECG_pin();

  ACK_param_setup();
  OFF_ACK_param();
  new_param_setup();
  
  /* COMMON CONFIGURATION */
  //************************** INTAN setup ************************** 

  stim_en_OFF();
  initialize_INTAN(&INTAN_config);
  configure_INTAN(&INTAN_config);
  // call_configuration_functions(&INTAN_config);
  // call_sense_configuration_functions(&INTAN_config, channel);
  // call_initialization_procedure_example(&INTAN_config);
  call_initialization_procedure_example_test_INTAN_functions(&INTAN_config);



uint8_t received_channel_value_1;
uint8_t received_channel_value_2;
// uint8_t received_channel_value_3;
// uint8_t received_channel_value_4;
enable_D_flag(&INTAN_config);

  while(1){

    switch (general_state) {
      case ENVIO_INTAN:{ // Estado 1: muestreo del INTAN y envío a través de SPI al ESP32 de los valores obtenidos en el INTAN

        new_param = new_param_read();
        if(new_param){
          general_state = RX_PARAMS_ESP32;
          break;
        }else{
          /*
            Send to the INTAN the convert command 
          */
            convert_channel(&INTAN_config, channel);
            send_SPI_commands_faster(&INTAN_config);
            received_channel_value_1 = (INTAN_config.obtained_RX[0] >> 24) & 0xFF;
            received_channel_value_2 = (INTAN_config.obtained_RX[0] >> 16) & 0xFF;
            // received_channel_value_3 = (INTAN_config.obtained_RX[0] >> 8)  & 0xFF;
            // received_channel_value_4 = INTAN_config.obtained_RX[0] & 0xFF;
            
            /*
              Resend to the ESP32 the received values from INTAN 
            */
            // // It is always sending data to the ESP32, if the timing pin is enabled then, the incorporated led will switch
            OFF_CS_ESP_PARAM_pin();
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            __delay_cycles(CLK_10_CYCLES);
            UCA0TXBUF = 0x31;
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            __delay_cycles(CLK_10_CYCLES);
            UCA0TXBUF = received_channel_value_1;
            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            __delay_cycles(CLK_10_CYCLES);
            UCA0TXBUF = received_channel_value_2;
            __delay_cycles(CLK_10_CYCLES);
            ON_CS_ESP_PARAM_pin();
        }
        break;
      }

      case RX_PARAMS_ESP32:{

        ON_ACK_param();
        __delay_cycles(1000);

        OFF_CS_ESP_PARAM_pin();

        // ----------------------------------------------------------------------
        // ADC sampling rate (2 bytes)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x10;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        uint8_t high_ADC_sampling_rate = UCA0RXBUF;

        UCA0TXBUF = 0x20;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        uint8_t low_ADC_sampling_rate = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Channels to convert (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x30;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config.number_channels_to_convert = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // DSP enabled (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x40;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config.DSPen = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // DSP cutoff frequency (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t dsp_bytes[4];

        UCA0TXBUF = 0x50;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x60;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x70;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x80;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Initial channel (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x90;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config.initial_channel_to_convert = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc high magnitude (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcH_bytes[4];

        UCA0TXBUF = 0xA0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0xB0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0xC0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0xD0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc high unit (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xE0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fc_high_unit = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc low A (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcA_bytes[4];

        UCA0TXBUF = 0xF0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x01;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x02;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x03;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc low B (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcB_bytes[4];

        UCA0TXBUF = 0x04;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x05;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x06;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x07;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Amplifier cutoff (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x08;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config.amplifier_cutoff_frequency_A_B[channel] = (char)UCA0RXBUF;


        // ----------------------------------------------------------------------
        // CLOSE chip select
        // ----------------------------------------------------------------------
        ON_CS_ESP_PARAM_pin();


        // ======================================================================
        // RECONSTRUCT 16-bit AND FLOAT PARAMETERS
        // ======================================================================

        // ADC sampling rate
        INTAN_config.ADC_sampling_rate =
            ((uint16_t)high_ADC_sampling_rate << 8) | low_ADC_sampling_rate;

        // DSP cutoff frequency (float)
        memcpy(&INTAN_config.DSP_cutoff_freq, dsp_bytes, 4);

        // fc high magnitude (float)
        memcpy(&INTAN_config.fh_magnitude, fcH_bytes, 4);

        // fc low A (float)
        memcpy(&INTAN_config.fc_low_A, fcA_bytes, 4);

        // fc low B (float)
        memcpy(&INTAN_config.fc_low_B, fcB_bytes, 4);

        general_state = NEW_PARAM_OFF_WAIT;
        break;


      }

      case NEW_PARAM_OFF_WAIT:{
        new_param = new_param_read();
        if(!new_param){
          general_state = TX_PARAMS_INTAN;
          break;
        }
        break;

      }

      case TX_PARAMS_INTAN:{
        general_state = ENVIO_INTAN;
        INTAN_function_update(&INTAN_config);
        OFF_ACK_param();
        break;  

      }

      default:
        break;
    }
  } 
}

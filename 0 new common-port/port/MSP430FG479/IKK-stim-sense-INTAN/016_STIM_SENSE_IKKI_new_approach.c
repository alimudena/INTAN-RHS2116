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
//                |  35         P5.3|-> STIMULATION ENABLED ------------------> - GPIO 9: D9
//                |  28         P5.1|-> HAND SHAKE SEND ----------------------> - GPIO 4: GPIO14
//                |  27         P5.0|-> HAND SHAKE READY ---------------------> - GPIO 5: D5
//                |  34         P5.2|-> HAND SHAKE ACK -----------------------> - GPIO 6: D6
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

/*
  STIMULATION UPDATE PARAMETERS
*/
uint16_t resting_time_miliseconds = 0;
uint8_t high_byte_resting_time_miliseconds = 0;
uint8_t low_byte_resting_time_miliseconds = 0;
uint16_t stimulation_time_miliseconds = 0;
uint8_t high_byte_stimulation_time_miliseconds = 0;
uint8_t low_byte_stimulation_time_miliseconds = 0;

uint8_t high_byte_stimulation_on_time_micro = 0;
uint8_t low_byte_stimulation_on_time_micro = 0;
uint16_t stimulation_on_time_micro = 0;
uint8_t high_stimulation_off_time_milis = 0;
uint8_t low_stimulation_off_time_milis = 0;
uint16_t stimulation_off_time_milis = 0;
uint8_t positive_current_magnitude = 0;
uint8_t negative_current_magnitude = 0;
uint8_t high_byte_step_DAC = 0;
uint8_t low_byte_step_DAC = 0;
uint16_t step_DAC = 0;

/*
  SENSE UPDATE PARAMETERS
*/
uint8_t high_ADC_sampling_rate;
uint8_t low_ADC_sampling_rate;
uint8_t high_ADC_sampling_rate_2;
uint8_t low_ADC_sampling_rate_2;
uint8_t high_ADC_sampling_rate_3;
uint8_t amplifier_cutoff;
uint8_t fc_high_unit;
char amplifier_cutoff_frequency_A_B;

uint8_t debug_send[6];
uint8_t debug_send_counter = 0;

uint32_t timeout;

uint16_t contador = 0;
uint16_t contador2 = 0;
uint8_t useless_variable;

typedef union {
    float value;
    uint8_t bytes[4];
} float_bytes_union;

#define N_SAMPLES 10

uint8_t received_channel_value_1_CH1[N_SAMPLES];
uint8_t received_channel_value_2_CH1[N_SAMPLES];
uint8_t received_channel_value_1_CH2[N_SAMPLES];
uint8_t received_channel_value_2_CH2[N_SAMPLES];

uint16_t n_samples = N_SAMPLES;

//Timer value configuration divider
uint32_t divider_value = 0x0016;

uint8_t RX;

bool new_parameters;
bool esp32_connected;
bool sent = false;
bool locked_button = false;
bool HS_value = false;
bool previous_HS_value = false;

/*
  STRUCTURES FOR CONFIGURATION OF DEVICES
*/
CLK_config_struct CLK_config;
SD16A_config_struct SD16A_configuration;
TIMER_config_struct TIMER_config;
INTAN_config_struct INTAN_config;
SPI_config_struct SPI_config;
SPI_B_config_struct SPI_B_config;


uint8_t channel_1 = 1;
uint8_t channel_2 = 2;

uint8_t stim_channel = 0;


//************************** other parameters ************************** 
bool new_param = false;

uint32_t stim_on_off_time;
uint32_t stim_rest_time;
bool next_stim;
bool previous_stim;

//************************** State machines ************************** 

typedef enum {ENVIO_INTAN, RX_PARAMS_ESP32, TX_PARAMS_INTAN, NEW_PARAM_OFF_WAIT} Estados;
Estados general_state = ENVIO_INTAN;


typedef enum {STIM, REST} states_stimulation;
states_stimulation state_stimulation = REST;

typedef enum {ON_P, OFF_P, ON_N, OFF_N} states_stimulation_ON_OFF;
states_stimulation_ON_OFF state_stimulation_ON_OFF = ON_P;

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

  INTAN_config->amplifier_cutoff_frequency_A_B[channel_1] = 'A';
  INTAN_config->amplifier_cutoff_frequency_A_B[channel_2] = 'A';


  /*
    CONSTANT CURRENT STIMULATOR
  */
  INTAN_config->step_DAC = 5000; // uA
  INTAN_config->negative_current_magnitude[stim_channel] = 100;
  INTAN_config->negative_current_trim[stim_channel] = 0x80;
  INTAN_config->positive_current_magnitude[stim_channel] = 100;
  INTAN_config->positive_current_trim[stim_channel] = 0x80;

  // INTAN_config->MASTER_FREQ = 8000000;


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

  INTAN_config->bipolar = false;
}

void receive_parameters_ESP32(INTAN_config_struct* INTAN_config){
    ON_ACK_param();
        __delay_cycles(1000);



        // ----------------------------------------------------------------------
        // OPEN chip select
        // ----------------------------------------------------------------------
        OFF_CS_ESP_PARAM_pin();
        
        /*
          SENSE RELATED PARAMETERS
        */

        // ----------------------------------------------------------------------
        // ADC sampling rate (2 bytes)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xB0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        uint8_t high_ADC_sampling_rate = UCA0RXBUF;

        UCA0TXBUF = 0xC0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        uint8_t low_ADC_sampling_rate = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Channels to convert (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xD0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->number_channels_to_convert = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // DSP enabled (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xE0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->DSPen = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // DSP cutoff frequency (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t dsp_bytes[4];

        UCA0TXBUF = 0xF0;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x01;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x02;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x03;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        dsp_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Initial channel (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x04;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->initial_channel_to_convert = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc high magnitude (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcH_bytes[4];

        UCA0TXBUF = 0x05;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x06;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x07;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x08;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcH_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc high unit (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x09;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fc_high_unit = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc low A (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcA_bytes[4];

        UCA0TXBUF = 0x0A;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x0B;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x0C;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x0D;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcA_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // fc low B (float → 4 bytes)
        // ----------------------------------------------------------------------
        uint8_t fcB_bytes[4];

        UCA0TXBUF = 0x0E;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[0] = UCA0RXBUF;

        UCA0TXBUF = 0x0F;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[1] = UCA0RXBUF;

        UCA0TXBUF = 0x11;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[2] = UCA0RXBUF;

        UCA0TXBUF = 0x12;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        fcB_bytes[3] = UCA0RXBUF;


        // ----------------------------------------------------------------------
        // Amplifier cutoff (1 byte)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x13;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->amplifier_cutoff_frequency_A_B[channel_1] = (char)UCA0RXBUF;
        INTAN_config->amplifier_cutoff_frequency_A_B[channel_2] = (char)UCA0RXBUF;

        // ----------------------------------------------------------------------
        // C2 enable
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x14;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->C2_enabled = (bool)UCA0RXBUF;

        // ----------------------------------------------------------------------
        // absolute value mode
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x15;
        while (!(IFG2 & UCA0RXIFG));
        __delay_cycles(30);
        INTAN_config->abs_mode = (bool)UCA0RXBUF;

        /*
          STRIMULATION RELATED PARAMETERS
        */

        // ---------------------------------------------------------------------- 
        // Number of stimulations
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x10;
        while (!(IFG2 & UCA0RXIFG));              // number of stimulations
        INTAN_config->number_of_stimulations = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // Resting time
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x20;
        while (!(IFG2 & UCA0RXIFG));              // resting_time_miliseconds
        high_byte_resting_time_miliseconds = UCA0RXBUF;
        
        UCA0TXBUF = 0x20;
        while (!(IFG2 & UCA0RXIFG));              // resting_time_miliseconds
        low_byte_resting_time_miliseconds = UCA0RXBUF;
        // ----------------------------------------------------------------------
        // Stimulation time
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x30;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_time_miliseconds
        high_byte_stimulation_time_miliseconds = UCA0RXBUF;

        UCA0TXBUF = 0x30;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_time_miliseconds
        low_byte_stimulation_time_miliseconds = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // Stimulation ON time (2 bytes)
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x40;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_on_time_micro high byte
        high_byte_stimulation_on_time_micro = UCA0RXBUF;
        UCA0TXBUF = 0x50;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_on_time_micro low byte
        low_byte_stimulation_on_time_micro = UCA0RXBUF;
        
        // ----------------------------------------------------------------------
        // Stimulation OFF time 
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x60;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_off_time
        high_stimulation_off_time_milis = UCA0RXBUF;

        UCA0TXBUF = 0x60;
        while (!(IFG2 & UCA0RXIFG));              // stimulation_off_time
        low_stimulation_off_time_milis = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // Positive current magnitude
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x70;
        while (!(IFG2 & UCA0RXIFG));              // positive_current_magnitude
        INTAN_config->positive_current_magnitude[stim_channel] = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // Negative current magnitude
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x80;
        while (!(IFG2 & UCA0RXIFG));              // negative_current_magnitude
        INTAN_config->negative_current_magnitude[stim_channel] = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // High step DAC
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0x90;
        while (!(IFG2 & UCA0RXIFG));              // high_byte_step_DAC
        high_byte_step_DAC = UCA0RXBUF;
        
        // ----------------------------------------------------------------------
        // Low step DAC
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xA0;
        while (!(IFG2 & UCA0RXIFG));              // low_byte_step_DAC
        low_byte_step_DAC = UCA0RXBUF;

        // ----------------------------------------------------------------------
        // Bipolar stimulation
        // ----------------------------------------------------------------------
        UCA0TXBUF = 0xA1;
        while (!(IFG2 & UCA0RXIFG));              // low_byte_step_DAC
        INTAN_config->bipolar = UCA0RXBUF;



        // ----------------------------------------------------------------------
        // CLOSE chip select
        // ----------------------------------------------------------------------
        ON_CS_ESP_PARAM_pin();




        // ======================================================================
        // RECONSTRUCT 16-bit AND FLOAT PARAMETERS
        // ======================================================================

        /*
          STIMULATION RELATED PARAMETERS
        */
        resting_time_miliseconds = ((uint16_t)high_byte_resting_time_miliseconds << 8) | low_byte_resting_time_miliseconds;
        stimulation_time_miliseconds = ((uint16_t)high_byte_stimulation_time_miliseconds << 8) | low_byte_stimulation_time_miliseconds;
        stimulation_off_time_milis = ((uint16_t)high_stimulation_off_time_milis << 8) | low_stimulation_off_time_milis;
        stimulation_on_time_micro = ((uint16_t)high_byte_stimulation_on_time_micro << 8) | low_byte_stimulation_on_time_micro;

        step_DAC = ((uint16_t)high_byte_step_DAC << 8) | low_byte_step_DAC;

        INTAN_config->MASTER_FREQ = 8000000;
        INTAN_config->resting_time = resting_time_miliseconds*INTAN_config->MASTER_FREQ/(divider_value*1000);
        INTAN_config->stimulation_time = (stimulation_time_miliseconds*INTAN_config->MASTER_FREQ)/(divider_value*1000);
        
        INTAN_config->stimulation_on_time = stimulation_on_time_micro*INTAN_config->MASTER_FREQ/(divider_value*1000000);
        INTAN_config->stimulation_off_time = stimulation_off_time_milis*INTAN_config->MASTER_FREQ/(100000*6*divider_value);

        INTAN_config->step_DAC = step_DAC; 


        /*
          SENSING RELATED PARAMETERS
        */
        // ADC sampling rate
        INTAN_config->ADC_sampling_rate =
            ((uint16_t)high_ADC_sampling_rate << 8) | low_ADC_sampling_rate;

        // DSP cutoff frequency (float)
        memcpy(&INTAN_config->DSP_cutoff_freq, dsp_bytes, 4);

        // fc high magnitude (float)
        memcpy(&INTAN_config->fh_magnitude, fcH_bytes, 4);

        // fc low A (float)
        memcpy(&INTAN_config->fc_low_A, fcA_bytes, 4);

        // fc low B (float)
        memcpy(&INTAN_config->fc_low_B, fcB_bytes, 4);

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

void config_CLK_8MHz(CLK_config_struct CLK_config){
  stop_wd();
  CLK_config.DCO_range = 4;
  CLK_config.N_MCLK = 121;
  CLK_config.DCOPLUS_on = true;
  CLK_config.D_val = 1;
  DCO_f_range(CLK_config.DCO_range);
  configure_N_for_MCLK (CLK_config.N_MCLK);
  LFXT1_internal_cap_config(18);
  FLL_CTL0 |= DCOPLUS;           // DCO+ set so freq= xtal x D x N+1  
  // configure_PINS_for_clk_debug();
}

int main(void)
{


  /* COMMON CONFIGURATION */
  
  general_setup(CLK_config);
  init_MSP();
  enable_interruptions(true);


  //************************** CLK configuration *****************************
  // config_CLK(&CLK_config);
  // run_functions_setup_CLK(CLK_config);
  config_CLK_8MHz(CLK_config);

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


  //************************** ESP32 indicator setup ************************** 
  ESP32_connected_setup();
  esp32_connected =  ESP32_connected();
  ESP32_LED_setup();
  stim_indicator_setup();

  //************************** SPI handshake indicator setup ************************** 
  HSHK_READY_setup();
  HSHK_ACK_setup();
  HSHK_ACK_high();
  HSHK_SEND_setup();
  HSHK_SEND_high();



  if(esp32_connected){
    /* NOT LOW POWER CONFIGURATION */
    //The ESP32 is connected so turn on its led
    ESP32_LED_setup();
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
    
    //************************** SD16 configuration *****************************
    // config_SD16A(&SD16A_configuration);
    // setup_SD16A(&SD16A_configuration);
    // start_conversion(); // While it is started, working in continuous mode will

    // SD16CCTL0 &= ~(SD16IE);   // Disabling SD16 interrupt
    IE2 &= ~(UCA0RXIE|UCA0TXIE);  // Disabling UART interrupt

    // CCTL0 = CCIE;                             // CCR0 interrupt enabled for timer

  }else{
    OFF_ESP32_LED();
  }


  /* COMMON CONFIGURATION */
  //************************** INTAN setup ************************** 

  stim_en_OFF();
  initialize_INTAN(&INTAN_config);
  configure_INTAN(&INTAN_config);
  

  call_configuration_functions(&INTAN_config);

  INTAN_config.single_shot = false;
  while(1){
    switch (general_state) {
      case ENVIO_INTAN:{ // Estado 1: Muestreo del ECG y envío a través de SPI al ESP32
        
        new_param = new_param_read();
        if(new_param){
          general_state = RX_PARAMS_ESP32;
          break;
        }else{

          /*
            Send to the INTAN the convert command for Channel 1
          */
          int samples_counter;
          new_param = new_param_read();
          if(!new_param){

            for (samples_counter = 0; samples_counter < n_samples; samples_counter++){
              new_param = new_param_read();
              if(new_param){
                general_state = RX_PARAMS_ESP32;
                break;
              }
              //CH 1
              convert_channel(&INTAN_config, channel_1);
              send_SPI_commands_faster(&INTAN_config);
              contador++;
              // received_channel_value_1_CH1[samples_counter] = (contador >> 8) & 0xFF;
              // received_channel_value_2_CH1[samples_counter] = (contador) & 0xFF;

              received_channel_value_1_CH1[samples_counter] = (INTAN_config.obtained_RX[0] >> 24) & 0xFF;
              received_channel_value_2_CH1[samples_counter] = (INTAN_config.obtained_RX[0] >> 16) & 0xFF;

              // received_channel_value_3 = (INTAN_config.obtained_RX[0] >> 8)  & 0xFF;
              // received_channel_value_4 = INTAN_config.obtained_RX[0] & 0xFF;
              
              //CH 2
              convert_channel(&INTAN_config, channel_2);
              send_SPI_commands_faster(&INTAN_config);
              contador2++;
              // received_channel_value_1_CH2[samples_counter] = (contador2 >> 8) & 0xFF;
              // received_channel_value_2_CH2[samples_counter] = (contador2) & 0xFF;

              received_channel_value_1_CH2[samples_counter] = (INTAN_config.obtained_RX[0] >> 24) & 0xFF;
              received_channel_value_2_CH2[samples_counter] = (INTAN_config.obtained_RX[0] >> 16) & 0xFF;

              // received_channel_value_3 = (INTAN_config.obtained_RX[0] >> 8)  & 0xFF;
              // received_channel_value_4 = INTAN_config.obtained_RX[0] & 0xFF;
            }
          }
          /*
            Resend to the ESP32 the received values from INTAN 
          */

          // Protocol for handshake constraints
          timeout = 10000;
          while(!HSHK_READY_value() && timeout--);
          
          HSHK_SEND_low();
          
          timeout = 10000;
          while(HSHK_READY_value() && timeout--);

          //End of protocol for handshake constraints
         
          // // It is always sending data to the ESP32, if the timing pin is enabled then, the incorporated led will switch
          OFF_CS_ESP_PARAM_pin();

          __delay_cycles(CLK_5_CYCLES);

          new_param = new_param_read();
          if(new_param){
            general_state = RX_PARAMS_ESP32;
            break;
          }

          for (samples_counter = 0; samples_counter < n_samples; samples_counter++){
            new_param = new_param_read();
            if(new_param){
              general_state = RX_PARAMS_ESP32;
              break;
            }
            
            while (!(IFG2 & UCA0TXIFG));              
            UCA0TXBUF = 0x31;
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;
            
            __delay_cycles(CLK_5_CYCLES);
            while (!(IFG2 & UCA0TXIFG));      
            UCA0TXBUF = received_channel_value_1_CH1[samples_counter];
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;

            __delay_cycles(CLK_5_CYCLES);
            while (!(IFG2 & UCA0TXIFG));              
            UCA0TXBUF = received_channel_value_2_CH1[samples_counter];
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;

            __delay_cycles(CLK_5_CYCLES);
            while (!(IFG2 & UCA0TXIFG));              
            UCA0TXBUF = 0x71;
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;

            __delay_cycles(CLK_5_CYCLES);
            while (!(IFG2 & UCA0TXIFG));              
            UCA0TXBUF = received_channel_value_1_CH2[samples_counter];
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;

            __delay_cycles(CLK_5_CYCLES);
            while (!(IFG2 & UCA0TXIFG));              
            UCA0TXBUF = received_channel_value_2_CH2[samples_counter];
            while (!(IFG2 & UCA0RXIFG)); 
            useless_variable=UCA0RXBUF;
            
            __delay_cycles(CLK_15_CYCLES);

          }

          ON_CS_ESP_PARAM_pin();

          HSHK_SEND_high();
          
          __delay_cycles(CLK_50_CYCLES);
         
          next_stim = button_pressed();
          if(next_stim){
            // if(!previous_stim){
              // previous_stim = true;
              number_of_stimulations_done = 0;
              state_stimulation = STIM;
              stim_rest_time = INTAN_config.stimulation_time;
              state_stimulation_ON_OFF = ON_P;
              timer_flag_stim_rest = 0;
              timer_flag_on_off = 0;
          }

          /*
            SEND DATA TO INTAN BECAUSE OF STATE CHANGE caused by a timer            
          */

          if(TACTL & TAIFG){ // POLLING AL BIT DE INTERRUPCION DEL TIMER
            TACTL &= ~TAIFG; // limpiar el flag

            timer_flag_stim_rest++;
            timer_flag_on_off++;

            // update_stim_parameters(); 
            if(INTAN_config.single_shot == false){
              if(number_of_stimulations_done < INTAN_config.number_of_stimulations-1){ // si el número de estimulaciones hechas es menor al que se quiere hacer
                // ON_pin();
                switch (state_stimulation) {
                  case STIM:
                    if(timer_flag_stim_rest >= stim_rest_time){
                      state_stimulation = REST;
                      timer_flag_stim_rest = 0;
                      stim_rest_time = INTAN_config.resting_time;
                      number_of_stimulations_done++;
                      INTAN_programmed = false;

                      break;
                    }
                      switch (state_stimulation_ON_OFF) {
                        case ON_P:

                          if(!INTAN_programmed){
                            stim_indicator_ON();
                            INTAN_config.stimulation_on[stim_channel] = 1;
                            INTAN_config.stimulation_pol[stim_channel] = 'P';
                            // ON_INTAN_FASTER(&INTAN_config, stim_channel);
                            ON_INTAN_FASTER_FASTER(&INTAN_config, stim_channel);
                            INTAN_programmed = true;
                          }
                          uint32_t timer_counter_value;
                          timer_counter_value = ((uint32_t)stimulation_on_time_micro/10) * ((uint32_t)INTAN_config.MASTER_FREQ / 1000000UL);
                          // wait for the stimulation time to be over.
                          while(timer_counter_value>0){
                            timer_counter_value--;
                            _no_operation();
                            // __delay_cycles(1);
                          }
                          OFF_INTAN_FASTER(&INTAN_config);   
                          state_stimulation_ON_OFF = OFF_P;
                          timer_flag_on_off = 0;
                          // stim_on_off_time = INTAN_config.stimulation_off_time;
                          INTAN_programmed = false;                        
                          break;

                        case OFF_P:
                          if(timer_flag_on_off > INTAN_config.stimulation_off_time){
                          if (INTAN_config.bipolar){
                            state_stimulation_ON_OFF = ON_N;
                          }else{
                            state_stimulation_ON_OFF = ON_P;
                          }
                            timer_flag_on_off = 0;
                            // stim_on_off_time = INTAN_config.stimulation_on_time;
                            INTAN_programmed = false;
                            break;
                          }          
                          if(!INTAN_programmed){
                            OFF_INTAN_FASTER(&INTAN_config);   
                            INTAN_programmed = true;
                            stim_indicator_OFF();
                          }
                          
                          
                          break;

                        case ON_N:
                          if(timer_flag_on_off > INTAN_config.stimulation_on_time){
                            state_stimulation_ON_OFF = OFF_N;
                            timer_flag_on_off = 0;
                            // stim_on_off_time = INTAN_config.stimulation_off_time;
                            INTAN_programmed = false;
                            break;
                          }
                          if(!INTAN_programmed){
                            INTAN_config.stimulation_on[stim_channel] = 1;
                            INTAN_config.stimulation_pol[stim_channel] = 'N';
                            // ON_INTAN_FASTER(&INTAN_config, stim_channel);
                            ON_INTAN_FASTER_FASTER(&INTAN_config, stim_channel);
                            stim_indicator_ON();
                            INTAN_programmed = true;
                          }

                          
                          break;

                        case  OFF_N:
                          if(timer_flag_on_off > INTAN_config.stimulation_off_time){
                            state_stimulation_ON_OFF = ON_P;
                            timer_flag_on_off = 0;
                            // stim_on_off_time = INTAN_config.stimulation_on_time;
                            INTAN_programmed = false;
                            break;
                          }
                          if(!INTAN_programmed){
                            OFF_INTAN_FASTER(&INTAN_config);
                            stim_indicator_OFF();
                            INTAN_programmed = true;
                          }
                          break;

                      }
                    break;

                  case REST:
                      if(timer_flag_stim_rest >= stim_rest_time){
                        state_stimulation = STIM;
                        timer_flag_stim_rest = 0;
                        stim_rest_time = INTAN_config.stimulation_time;
                        state_stimulation_ON_OFF = ON_P;
                        INTAN_programmed = false;
                        break;
                      }
                      if(!INTAN_programmed){
                        stim_indicator_OFF();
                        OFF_INTAN_FASTER(&INTAN_config);
                        INTAN_programmed = true;
                      }
                    break;
                
                }
              }else{
                next_stim = button_pressed();
                OFF_pin();
              }
            }else{

              next_stim = button_pressed();
              if(next_stim){
                if(!locked_button){
                  INTAN_programmed = false;
                  timer_flag_on_off = 0;
                }
                locked_button = true;
              }else{
                locked_button = false;
              }
              if(!INTAN_programmed){
                INTAN_config.stimulation_on[stim_channel] = 1;
                INTAN_config.stimulation_pol[stim_channel] = 'P';
                // ON_INTAN_FASTER(&INTAN_config, stim_channel);
                ON_INTAN_FASTER_FASTER(&INTAN_config, stim_channel);
                INTAN_programmed = true;
              }
              if(timer_flag_on_off >=  INTAN_config.stimulation_on_time){
                OFF_INTAN_FASTER(&INTAN_config);
                break;
              }   
            }
          }
        }
        break;
      } 
   

      case RX_PARAMS_ESP32:{// Estado 2: Recepción de parámetros de estimulación nuevos a través del ESP32
        receive_parameters_ESP32(&INTAN_config);
        general_state = NEW_PARAM_OFF_WAIT;
        break;
      } 
      
      case NEW_PARAM_OFF_WAIT:{ // Estado 3: Espera a la bajada del pinde comunicaciones
        new_param = new_param_read();
        if(!new_param){
          general_state = TX_PARAMS_INTAN;
          break;
        }
        break;
      }

      case TX_PARAMS_INTAN:{ // Estado 4: actualización de parámetros de estimulación del INTAN
        general_state = ENVIO_INTAN;
        number_of_stimulations_done = 100;

        // The next both functions do the same but are defined a little different
        // call_configuration_functions(&INTAN_config);
        INTAN_function_update(&INTAN_config);


        OFF_ACK_param();
        break;        
      }

      default:
        break;

    }

    // Comprobar los flags del timer y actualizar parámetros del INTAN si es necesario
  
  }




}
// Timer A0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMERA0_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{

  timer_flag_stim_rest++;
  timer_flag_on_off++;
  // button_init();
  // next_stim = button_pressed();
  // if(next_stim){
  //   number_of_stimulations_done = 0;
  //   state_stimulation = STIM;
  // }

  // update_stim_parameters();
}

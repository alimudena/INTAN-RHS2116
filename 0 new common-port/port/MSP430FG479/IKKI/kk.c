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
        bool next_stim = button_pressed(); 
                initialize_INTAN(&INTAN_config);

        while(1){
                while(!next_stim){
                        next_stim = button_pressed();
                }
                read_command(&INTAN_config, 255, '1');
             
                send_SPI_commands(&INTAN_config);
             

                /* 
                        STIMULATION DISABLE AND MINIMUM POWER DISIPATION
                */
                
                stimulation_disable(&INTAN_config); // 0x0000 0x0000
                send_SPI_commands(&INTAN_config);
                minimum_power_disipation(&INTAN_config);
                clear_command(&INTAN_config);
                INTAN_config.ADC_sampling_rate = 480;
                ADC_sampling_rate_config(&INTAN_config);

                send_SPI_commands(&INTAN_config);
             
                /*
                        AUXILIARY DIGITAL OUTPUTS
                */
                disable_digital_output_1(&INTAN_config);
                disable_digital_output_2(&INTAN_config);
                power_OFF_output_1(&INTAN_config);
                power_OFF_output_2(&INTAN_config);
                /*
                        ABSOLUTE VALUE MODE
                */
                disable_absolute_value(&INTAN_config);
                /*
                        DSP FOR HIGH PASS FILTER REMOVAL
                */
                disable_digital_signal_processing_HPF(&INTAN_config);
                INTAN_config.DSP_cutoff_freq = 4.665;
                INTAN_config.number_channels_to_convert = 8;
                DSP_cutoff_frequency_configuration(&INTAN_config);
                /*
                        GENERAL
                */
                disable_C2(&INTAN_config);

                send_SPI_commands(&INTAN_config);

                /*
                        ELECTRODE IMPEDANCE TEST
                */
                INTAN_config.zcheck_select = 0;
                INTAN_config.zcheck_load = 1;
                INTAN_config.zcheck_scale = 0;
                INTAN_config.zcheck_en = 0;
                impedance_check_control(&INTAN_config);
                INTAN_config.zcheck_DAC_value = 128;
                impedance_check_DAC(&INTAN_config);

                send_SPI_commands(&INTAN_config);

                /*
                        AMPLIFIER BANDWIDTH
                */

                INTAN_config.fh_magnitude = 7.5;
                INTAN_config.fh_unit = 'k';
                INTAN_config.fc_low_A = 5;
                INTAN_config.fc_low_B = 1000;
                
                fc_high(&INTAN_config);
                fc_low_A(&INTAN_config);
                fc_low_B(&INTAN_config);
                amp_fast_settle_reset(&INTAN_config);          
                A_or_B_cutoff_frequency(&INTAN_config);

                send_SPI_commands(&INTAN_config);

             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                INTAN_config.step_DAC = 1000; // uA
                stim_step_DAC_configuration(&INTAN_config);
                stim_PNBIAS_configuration(&INTAN_config);    

                send_SPI_commands(&INTAN_config);

                /*
                        CURRENT LIMITED CHARGE RECOVERY CIRCUIT
                */             
            
                INTAN_config.voltage_recovery = 0;
                INTAN_config.current_recovery = 1;
                charge_recovery_voltage_configuration(&INTAN_config);
                charge_recovery_current_configuration(&INTAN_config);
                send_SPI_commands(&INTAN_config);
             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                all_stim_channels_off(&INTAN_config);
                stimulation_on(&INTAN_config);
                stimulation_polarity(&INTAN_config);
                /*
                        CHARGE RECOVERY SWITCH
                */
                disconnect_channels_from_gnd(&INTAN_config);
                /*
                        CURRENT LIMITED CHARGE RECOVERY CIRCUIT
                */
                disable_charge_recovery_sw(&INTAN_config);
                send_SPI_commands(&INTAN_config);
             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                stim_current_channel_configuration(&INTAN_config, 0, 0x80, 0xFF, 0x80, 0xFF);
                for (i = NUM_CHANNELS-1; i>0; i--){
                        stim_current_channel_configuration(&INTAN_config, i, 0x80, 0x00, 0x80, 0x00);
                }

                send_SPI_commands(&INTAN_config);


                /*
                        U AND M FLAGS
                */
                enable_U_flag(&INTAN_config);
                enable_M_flag(&INTAN_config);
                read_command(&INTAN_config, 255, 'E');
                disable_U_flag(&INTAN_config);
                disable_M_flag(&INTAN_config);

                send_SPI_commands(&INTAN_config);




                //The chip is now initialized

                #define NUM_STIM_EXP 3
                #define frequencia_de_estimulacion 20 //20 Hz 
                #define tiempo_estimulacion 30 //30 segundos
                
                #define T_total 1.0f/frequencia_de_estimulacion //1/20Hz
                #define N_T_total (tiempo_estimulacion * frequencia_de_estimulacion)

                #define TIEMPO_ON_POSITIVO 500 //0.5ms
                #define TIEMPO_ON_NEGATIVO 500 //0.5ms

                #define CLK_TIEMPO_ON_POSITIVO TIEMPO_ON_POSITIVO*FREQ_MASTER/1000000
                #define CLK_TIEMPO_ON_NEGATIVO TIEMPO_ON_NEGATIVO*FREQ_MASTER/1000000

                #define TIEMPO_OFF (T_total-TIEMPO_ON_POSITIVO/1000000-TIEMPO_ON_NEGATIVO/1000000)/2
                #define CLK_TIEMPO_OFF TIEMPO_OFF*FREQ_MASTER



                uint8_t numero_stim_experimento_loop;
                uint16_t numero_stim_pos_neg_loop;
                for(numero_stim_experimento_loop = NUM_STIM_EXP; numero_stim_experimento_loop>0; numero_stim_experimento_loop--){
                        for(numero_stim_pos_neg_loop = N_T_total; numero_stim_pos_neg_loop>0; numero_stim_pos_neg_loop--){
                                // estimulacion positiva
                                INTAN_config.stimulation_on[0] = 1;
                                INTAN_config.stimulation_pol[0] = 'P';
                                ON_INTAN(&INTAN_config);
                                __delay_cycles(CLK_TIEMPO_ON_POSITIVO);                         

                                // no estimulacion
                                OFF_INTAN(&INTAN_config);
                                __delay_cycles(CLK_TIEMPO_OFF);                       

                                // estimulacion negativa
                                INTAN_config.stimulation_pol[0] = 'N';
                                ON_INTAN(&INTAN_config);
                                __delay_cycles(CLK_TIEMPO_ON_NEGATIVO);                             

                                // no estimulacion
                                OFF_INTAN(&INTAN_config);
                                __delay_cycles(CLK_TIEMPO_OFF);                              

                        }
                        __delay_cycles(CLK_5_M_CYCLES); 
                }



        }     


}





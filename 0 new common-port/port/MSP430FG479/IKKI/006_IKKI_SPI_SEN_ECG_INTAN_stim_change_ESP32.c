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
#include "../../../common/INTAN_functions.h"
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
        bool next_stim = button_pressed(); 

        //************************** ESP32 timing control setup  ************************** 
        timing_control_ESP_init();
        stimulation_enable_ESP_init();
        new_parameters_ESP_init();

        //************************** ESP32 communication setup ************************** 
        CS_ESP_setup();
        ON_CS_ESP_pin();

        //************************** SD16 configuration *****************************
        config_SD16A(&SD16A_configuration);
        setup_SD16A(&SD16A_configuration);
        start_conversion(); // While it is started, working in continuous mode will

        SD16CCTL0 &= ~(SD16IE);   // Disabling SD16 interrupt
        IE2 &= ~(UCA0RXIE|UCA0TXIE);  // Disabling UART interrupt

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

        /*State machine for stimulation parameters*/
        /*

                state 0: positive stimulation
                state 1: neutral stimulation
                state 3: negative stimulation
                state 4: neutral stimulation


        
        */

        //************************** other parameters ************************** 
        volatile uint8_t RX;
        volatile uint8_t TX = 0;
        bool state_changed = false; // for controlling only one state time is changed
        bool stimulation_disabled = false; 
        bool order_sent = false; // for controlling only one order is sent in every change of state machine
        uint8_t state = 0;

        while(1){
                // while(!next_stim){
                //         next_stim = button_pressed();
                // }
                
                /*
                        SEND TO ESP32 THE READ VALUES FROM ECG
                */

                // It is always sending data to the ESP32, if the timing pin is enabled then, the incorporated led will switch
                OFF_CS_ESP_pin();
                /*
                        We try only to send one value by SPI to the ESP32 every time we press the button
                */
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

               
                ON_CS_ESP_pin();

                /*
                        SEND DATA TO INTAN BECAUSE OF TIMING CHANGE: change of status
                        
                */
                // LED control of on and off - pasará a ser el envío a través del intan
                volatile bool stimulation_enabled_bool = stimulation_enable_ESP(); 
                volatile bool timing_control_bool = timing_control_ESP();
                if(stimulation_enabled_bool){ // Si está habilitada la estimulación: se va cambiando de estado
                        stimulation_disabled = false;
                        ON_pin(); // If stimulation happening turn on led
                        if(timing_control_bool){ // Y se ha mandado una alerta desde el ESP32 para cambiar los parametros de estimulación
                                if(!state_changed){    
                                        // Change the state
                                        state_changed = true; 
                                        switch (state) {
                                                case 0: // positive stimulation
                                                if(!order_sent){
                                                        state = 1;
                                                        INTAN_config.stimulation_on[0] = 1;
                                                        INTAN_config.stimulation_pol[0] = 'P';
                                                        ON_INTAN(&INTAN_config);
                                                        order_sent = true;
                                                }
                                                break;  

                                                case 1: // neutral stimulation
                                                if(!order_sent){
                                                        state = 2;
                                                        OFF_INTAN(&INTAN_config);
                                                        order_sent = true;
                                                }
                                                break;  

                                                case 2: // negative stimulation
                                                if(!order_sent){
                                                        state = 3;
                                                        INTAN_config.stimulation_pol[0] = 'N';
                                                        ON_INTAN(&INTAN_config);
                                                        order_sent = true;
                                                }
                                                break;  

                                                case 3: // neutral stimulation
                                                if(!order_sent){
                                                        state = 0;
                                                        OFF_INTAN(&INTAN_config);
                                                        order_sent = true;
                                                }
                                                break;  
                                                
                                                default:
                                                        perror("Error: CASE SELECTION WRONG.");
                                                        break;  
                                        }
                                }
                        }else{
                                state_changed = false;
                                order_sent = false;

                        }
                }else{ // stimulation off and led indicating stimulation off
                        if(!stimulation_disabled){
                                OFF_pin();
                                OFF_INTAN(&INTAN_config);
                                stimulation_disabled = true;
                        }
                }

        }

}



#include "INTAN_config.h"

#include "values_macro.h"
#include "register_macro.h"

#include <math.h>
#include <stdbool.h> 
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Necesario para uint16_t
#include "../port/MSP430FG479/functions/general_functions.h"



uint16_t unify_8bits(uint8_t high, uint8_t low) {
    return ((uint16_t)high << 8) | low;
}


uint32_t unify_16bits(uint16_t high, uint16_t low) {
    return ((uint32_t)high << 16) | low;
}
/*
    Functions for enabling and disabling the booleans described in the INTAN for MSP430
*/

void enable_M_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->M_flag = 1;
}
void enable_U_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->U_flag = 1;
}
void enable_H_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->H_flag = 1;
}
void enable_D_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->D_flag = 1;
}

void disable_M_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->M_flag = 0;
}
void disable_U_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->U_flag = 0;
}
void disable_H_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->H_flag = 0;
}
void disable_D_flag(INTAN_config_struct *INTAN_config){
    INTAN_config->D_flag = 0;
}




/*
    Function for initializing INTAN structure values
*/

void initialize_INTAN(INTAN_config_struct* INTAN_config){
    disable_M_flag(INTAN_config);
    disable_U_flag(INTAN_config);
    disable_H_flag(INTAN_config);
    disable_D_flag(INTAN_config);

    INTAN_config->max_size = 0;
    unsigned i;
    for (i= MAX_VALUES; i > 0; i--) {
        INTAN_config->expected_RX[i-1] = 0;
        INTAN_config->obtained_RX[i-1] = 0;
        INTAN_config->expected_RX_bool[i-1] = 0;    
        INTAN_config->time_restriction[i-1] = 0;

    }
    
    // Register 1
    INTAN_config->digoutOD = false;
    INTAN_config->digout2 = false;
    INTAN_config->digout2HZ = false;
    INTAN_config->digout1 = false;
    INTAN_config->digout1HZ = false;

    INTAN_config->weak_MISO = false;

    INTAN_config->C2_enabled = false;

    INTAN_config->abs_mode = false;
    INTAN_config->DSPen = false;
    INTAN_config->DSP_cutoff_freq = 1;
    
    
    INTAN_config->initial_channel_to_convert = 3;
    INTAN_config->step_DAC = 10;
    INTAN_config->waiting_trigger = false;
    
    INTAN_config->current_recovery = 20;
    INTAN_config->voltage_recovery = -1.225;
    INTAN_config->ADC_sampling_rate = 100;
    
    
    INTAN_config->zcheck_select = 0;
    INTAN_config->zcheck_DAC_enhable = 0;
    INTAN_config->zcheck_load = 0;
    INTAN_config->zcheck_scale = 0;
    INTAN_config->zcheck_en = 0;

    INTAN_config->zcheck_DAC_value = 0;

    
    INTAN_config->fh_magnitude = 7.5;
    INTAN_config->fh_unit = 'k';
    INTAN_config->fc_low_A = 10;
    INTAN_config->fc_low_B = 10;
    for (i = NUM_CHANNELS; i > 0; i--){
        INTAN_config->amplifier_cutoff_frequency_A_B[i-1] = 'A';
        INTAN_config->stimulation_on[i-1] = false;
        INTAN_config->stimulation_pol[i-1] = 'P';
        // INTAN_config->negative_current_magnitude[i-1] = 0x00;
        // INTAN_config->negative_current_trim[i-1] = 0x80;
        // INTAN_config->positive_current_magnitude[i-1] = 0x00;
        // INTAN_config->positive_current_trim[i-1] = 0x80;
 
    }

}

// Function used for sending SPI commands

/*While not all the commands have been sent:
    State 1:
        Update the packets to be sent in state 2
    State 2:
        Send the 4 packets and save the received values in the arrays for checking the values

  When all the commands have been sent:
    Check that the received values are correct using the function for checking it. 
    If the returned value of the function is false:
        enter a while loop and turn on the led for the INTAN configuration warning
    Else: exit the function

  */  
void send_SPI_commands(INTAN_config_struct* INTAN_config){

    /*
    Se añade a la lista de envíos dos dummies para que se puedan comprobar todas las recepciones tras los envíos.
    Si no se hace esto entonces habría que implementar una lógica adicional de traslado de paquetes hacia el inicio de los que se han recibido y 
    comprobarlo más adelante, aciendo que el fallo venga más tarde de lo deseado y pudiendo producir un error indeseado como una configuración mala.
    */
    send_confirmation_values(INTAN_config);
    uint16_t pckt_count = 0;

    while(pckt_count != INTAN_config->max_size){       
            
            // wait_1_second();
            
            OFF_CS_pin();
            __delay_cycles(CLK_5_CYCLES);

            send_values(INTAN_config, pckt_count);
            
            __delay_cycles(CLK_5_CYCLES);
            ON_CS_pin();   
            
            __delay_cycles(CLK_10_CYCLES);

            if(INTAN_config->time_restriction[pckt_count]){
                wait_1_second();
            }
            pckt_count += 1;
        
    }
    pckt_count = 0;
    
    bool checked_rcvd = check_received_commands(INTAN_config);
    if (!checked_rcvd){
        // while(1){
            // ON_INTAN_LED();
            // perror("ERROR RECEIVING.");
        // }
    }else{
         OFF_INTAN_LED();
    }
    INTAN_config->max_size = 0;
}

// Funtion used to check if the returned values are correct or not
bool check_received_commands(INTAN_config_struct* INTAN_config){
    uint16_t index;
    // If the value to be compared actually does not mean anything (as if it a read function) return true
    for (index = 0; index < INTAN_config->max_size-2; index++){ //-2 because 2 extra packets are sent
        if (INTAN_config->expected_RX_bool[index] == 1) {       
            if (index+2 >= MAX_VALUES){
                // Failure because more than the expected values have been sent and there is an index out of bounds.
                return false;
            }
            if (INTAN_config->obtained_RX[index+2] != INTAN_config->expected_RX[index]){
                INTAN_config->obtained_RX[index+2] = 0;
                INTAN_config->expected_RX[index] = 0;
                return false;
            }
                INTAN_config->obtained_RX[index+2] = 0;
                INTAN_config->expected_RX[index] = 0;
        }
        if (INTAN_config->instruction[index] == 'M'){
            int i;
            for (i = 0; i < 16; i++) {
                // Compare bit a bit if any has a value 1 this one is related to the reading of the register 40 in the compliance monitor
                if (INTAN_config->obtained_RX[index+2] & (1 << i)) {
                    perror("Initialyzing compliance monitor.");
                }
            }
          
        } else if (INTAN_config->instruction[index] == 'f') {
            // Compare bit zero if has a value 1 this one is related to the reading of the register 50 fault current detector
            if (INTAN_config->obtained_RX[0] & (1)) {
                perror("Fault current detection.");
                perror("Rewrite SPI commands or turn off.");
                while(1);
            }

        }


    }

    return true;
}

void fc_high(INTAN_config_struct* INTAN_config){
    uint8_t RH1_sel1;
    uint8_t RH1_sel2;
    uint8_t RH2_sel1;
    uint8_t RH2_sel2;

    uint16_t RH1;
    uint16_t RH2;

    char H_k = INTAN_config->fh_unit;
    float_t freq = INTAN_config->fh_magnitude;    


    switch (H_k) {
        case 'k':
             if (freq == 20.0f){
                    RH1_sel1 = RH1_SEL1_20K;
                    RH1_sel2 = RH1_SEL2_20K;
                    RH2_sel1 = RH2_SEL1_20K;
                    RH2_sel2 = RH2_SEL2_20K;
                    }
              else if (freq == 15.0f){
                    RH1_sel1 = RH1_SEL1_15K;
                    RH1_sel2 = RH1_SEL2_15K;
                    RH2_sel1 = RH2_SEL1_15K;
                    RH2_sel2 = RH2_SEL2_15K;
                    }
              else if (freq == 10.0f){
                    RH1_sel1 = RH1_SEL1_10K;
                    RH1_sel2 = RH1_SEL2_10K;
                    RH2_sel1 = RH2_SEL1_10K;
                    RH2_sel2 = RH2_SEL2_10K;
                    }
              else if (freq == 7.5f){
                    RH1_sel1 = RH1_SEL1_7_5K;
                    RH1_sel2 = RH1_SEL2_7_5K;
                    RH2_sel1 = RH2_SEL1_7_5K;
                    RH2_sel2 = RH2_SEL2_7_5K;
                    }
              else if (freq == 5.0f){
                    RH1_sel1 = RH1_SEL1_5K;
                    RH1_sel2 = RH1_SEL2_5K;
                    RH2_sel1 = RH2_SEL1_5K;
                    RH2_sel2 = RH2_SEL2_5K;
                    }
              else if (freq == 3.0f){
                    RH1_sel1 = RH1_SEL1_3K;
                    RH1_sel2 = RH1_SEL2_3K;
                    RH2_sel1 = RH2_SEL1_3K;
                    RH2_sel2 = RH2_SEL2_3K;
                    }
              else if (freq == 2.5f){
                    RH1_sel1 = RH1_SEL1_2_5K;
                    RH1_sel2 = RH1_SEL2_2_5K;
                    RH2_sel1 = RH2_SEL1_2_5K;
                    RH2_sel2 = RH2_SEL2_2_5K;
                    }
              else if (freq == 2.0f){
                    RH1_sel1 = RH1_SEL1_2K;
                    RH1_sel2 = RH1_SEL2_2K;
                    RH2_sel1 = RH2_SEL1_2K;
                    RH2_sel2 = RH2_SEL2_2K;
                    }
              else if (freq == 1.5f){
                    RH1_sel1 = RH1_SEL1_1_5K;
                    RH1_sel2 = RH1_SEL2_1_5K;
                    RH2_sel1 = RH2_SEL1_1_5K;
                    RH2_sel2 = RH2_SEL2_1_5K;
                    }
              else if (freq == 1.0f){
                    RH1_sel1 = RH1_SEL1_1K;
                    RH1_sel2 = RH1_SEL2_1K;
                    RH2_sel1 = RH2_SEL1_1K;
                    RH2_sel2 = RH2_SEL2_1K;
                }
                
               else{     
                    perror("Error: high cutoff frequency Kilo.");
                        
               }
            break;

        case 'H':
                if (freq == 750){
                    RH1_sel1 = RH1_SEL1_750H;
                    RH1_sel2 = RH1_SEL2_750H;
                    RH2_sel1 = RH2_SEL1_750H;
                    RH2_sel2 = RH2_SEL2_750H;
                    }
                else if (freq == 500){
                    RH1_sel1 = RH1_SEL1_500H;
                    RH1_sel2 = RH1_SEL2_500H;
                    RH2_sel1 = RH2_SEL1_500H;
                    RH2_sel2 = RH2_SEL2_500H;
                    }
                else if (freq == 300){
                    RH1_sel1 = RH1_SEL1_300H;
                    RH1_sel2 = RH1_SEL2_300H;
                    RH2_sel1 = RH2_SEL1_300H;
                    RH2_sel2 = RH2_SEL2_300H;
                    }
                else if (freq == 250){
                    RH1_sel1 = RH1_SEL1_250H;
                    RH1_sel2 = RH1_SEL2_250H;
                    RH2_sel1 = RH2_SEL1_250H;
                    RH2_sel2 = RH2_SEL2_250H;
                    }
                else if (freq == 200){
                    RH1_sel1 = RH1_SEL1_200H;
                    RH1_sel2 = RH1_SEL2_200H;
                    RH2_sel1 = RH2_SEL1_200H;
                    RH2_sel2 = RH2_SEL2_200H;
                    }
                else if (freq == 150){
                    RH1_sel1 = RH1_SEL1_150H;
                    RH1_sel2 = RH1_SEL2_150H;
                    RH2_sel1 = RH2_SEL1_150H;
                    RH2_sel2 = RH2_SEL2_150H;
                    }
                else if (freq == 100){
                    RH1_sel1 = RH1_SEL1_100H;
                    RH1_sel2 = RH1_SEL2_100H;
                    RH2_sel1 = RH2_SEL1_100H;
                    RH2_sel2 = RH2_SEL2_100H;}
                else{
                    perror("Error: high cutoff frequency H.");
                    }    
            break;
        default:
            perror("Error: high cutoff frequency units.");
            break;    
    }
    RH1 = ((uint16_t)RH1_sel2 << 6) | RH1_sel1;
    RH2 = ((uint16_t)RH2_sel2 << 6) | RH2_sel1;

    // RH1 = unify_8bits(RH1_sel2, RH1_sel1);   
    // RH2 = unify_8bits(RH2_sel2, RH2_sel1);

    // Calls write command with the corresponding registers and frequency values
    write_command(INTAN_config, (uint8_t)ADC_HIGH_FREQ_4, RH1);
    write_command(INTAN_config, (uint8_t)ADC_HIGH_FREQ_5, RH2);

}

void fc_low_A(INTAN_config_struct* INTAN_config){
    uint8_t RL_SEL1;
    uint8_t RL_SEL2;
    uint8_t RL_SEL3;

    uint16_t RL;
    float_t freq = INTAN_config-> fc_low_A;
    if (freq == 1000.0f){
        RL_SEL1 = RL_SEL1_1k;
        RL_SEL2 = RL_SEL2_1k;
        RL_SEL3 = RL_SEL3_1k;

    }else if(freq == 500.0f){
        RL_SEL1 = RL_SEL1_500H;
        RL_SEL2 = RL_SEL2_500H;
        RL_SEL3 = RL_SEL3_500H;

    }else if(freq == 300.0f){
        RL_SEL1 = RL_SEL1_300H;
        RL_SEL2 = RL_SEL2_300H;
        RL_SEL3 = RL_SEL3_300H;
        
    }else if(freq == 250.0f){
        RL_SEL1 = RL_SEL1_250H;
        RL_SEL2 = RL_SEL2_250H;
        RL_SEL3 = RL_SEL3_250H;
        
    }else if(freq == 200.0f){
        RL_SEL1 = RL_SEL1_200H;
        RL_SEL2 = RL_SEL2_200H;
        RL_SEL3 = RL_SEL3_200H;
        
    }else if(freq == 150.0f){
        RL_SEL1 = RL_SEL1_150H;
        RL_SEL2 = RL_SEL2_150H;
        RL_SEL3 = RL_SEL3_150H;
        
    }else if(freq == 100.0f){
        RL_SEL1 = RL_SEL1_100H;
        RL_SEL2 = RL_SEL2_100H;
        RL_SEL3 = RL_SEL3_100H;
        
    }else if(freq == 75.0f){
        RL_SEL1 = RL_SEL1_75H;
        RL_SEL2 = RL_SEL2_75H;
        RL_SEL3 = RL_SEL3_75H;
        
    }else if(freq == 50.0f){
        RL_SEL1 = RL_SEL1_50H;
        RL_SEL2 = RL_SEL2_50H;
        RL_SEL3 = RL_SEL3_50H;
        
    }else if(freq == 30.0f){
        RL_SEL1 = RL_SEL1_30H;
        RL_SEL2 = RL_SEL2_30H;
        RL_SEL3 = RL_SEL3_30H;
        
    }else if(freq == 25.0f){
        RL_SEL1 = RL_SEL1_25H;
        RL_SEL2 = RL_SEL2_25H;
        RL_SEL3 = RL_SEL3_25H;
        
    }else if(freq == 20.0f){
        RL_SEL1 = RL_SEL1_20H;
        RL_SEL2 = RL_SEL2_20H;
        RL_SEL3 = RL_SEL3_20H;
        
    }else if(freq == 15.0f){
        RL_SEL1 = RL_SEL1_15H;
        RL_SEL2 = RL_SEL2_15H;
        RL_SEL3 = RL_SEL3_15H;
        
    }else if(freq == 10.0f){
        RL_SEL1 = RL_SEL1_10H;
        RL_SEL2 = RL_SEL2_10H;
        RL_SEL3 = RL_SEL3_10H;
        
    }else if(freq == 7.5f){
        RL_SEL1 = RL_SEL1_7_5H;
        RL_SEL2 = RL_SEL2_7_5H;
        RL_SEL3 = RL_SEL3_7_5H;
        
    }else if(freq == 5.0f){
        RL_SEL1 = RL_SEL1_5H;
        RL_SEL2 = RL_SEL2_5H;
        RL_SEL3 = RL_SEL3_5H;
        
    }else if(freq == 3.0f){
        RL_SEL1 = RL_SEL1_3H;
        RL_SEL2 = RL_SEL2_3H;
        RL_SEL3 = RL_SEL3_3H;
        
    }else if(freq == 2.5f){
        RL_SEL1 = RL_SEL1_2_5H;
        RL_SEL2 = RL_SEL2_2_5H;
        RL_SEL3 = RL_SEL3_2_5H;
        
    }else if(freq == 2.0f){
        RL_SEL1 = RL_SEL1_2H;
        RL_SEL2 = RL_SEL2_2H;
        RL_SEL3 = RL_SEL3_2H;
        
    }else if(freq == 1.5f){
        RL_SEL1 = RL_SEL1_1_5H;
        RL_SEL2 = RL_SEL2_1_5H;
        RL_SEL3 = RL_SEL3_1_5H;
        
    }else if(freq == 1.0f){
        RL_SEL1 = RL_SEL1_1H;
        RL_SEL2 = RL_SEL2_1H;
        RL_SEL3 = RL_SEL3_1H;
        
    }else if(freq == 0.75f){
        RL_SEL1 = RL_SEL1_0_75H;
        RL_SEL2 = RL_SEL2_0_75H;
        RL_SEL3 = RL_SEL3_0_75H;
        
    }else if(freq == 0.5f){
        RL_SEL1 = RL_SEL1_0_5H;
        RL_SEL2 = RL_SEL2_0_5H;
        RL_SEL3 = RL_SEL3_0_5H;
        
    }else if(freq == 0.3f){
        RL_SEL1 = RL_SEL1_0_3H;
        RL_SEL2 = RL_SEL2_0_3H;
        RL_SEL3 = RL_SEL3_0_3H;
        
    }else if(freq == 0.25f){
        RL_SEL1 = RL_SEL1_0_25H;
        RL_SEL2 = RL_SEL2_0_25H;
        RL_SEL3 = RL_SEL3_0_25H;
        
    }else if(freq == 0.1f){
        RL_SEL1 = RL_SEL1_0_1H;
        RL_SEL2 = RL_SEL2_0_1H;
        RL_SEL3 = RL_SEL3_0_1H;
        
    }else{
        perror("Error: high cutoff frequency Kilo.");
    }
    RL = ((uint16_t)RL_SEL3 << 13) |((uint16_t)RL_SEL2 << 7) | RL_SEL1;
    write_command(INTAN_config, (uint8_t)ADC_LOW_FREQ_A, RL);

}

void fc_low_B(INTAN_config_struct* INTAN_config){
    uint8_t RL_SEL1;
    uint8_t RL_SEL2;
    uint8_t RL_SEL3;

    uint16_t RL;

    float_t freq = INTAN_config-> fc_low_B;

    if (freq == 1000.0f){
        RL_SEL1 = RL_SEL1_1k;
        RL_SEL2 = RL_SEL2_1k;
        RL_SEL3 = RL_SEL3_1k;

    }else if(freq == 500.0f){
        RL_SEL1 = RL_SEL1_500H;
        RL_SEL2 = RL_SEL2_500H;
        RL_SEL3 = RL_SEL3_500H;

    }else if(freq == 300.0f){
        RL_SEL1 = RL_SEL1_300H;
        RL_SEL2 = RL_SEL2_300H;
        RL_SEL3 = RL_SEL3_300H;
        
    }else if(freq == 250.0f){
        RL_SEL1 = RL_SEL1_250H;
        RL_SEL2 = RL_SEL2_250H;
        RL_SEL3 = RL_SEL3_250H;
        
    }else if(freq == 200.0f){
        RL_SEL1 = RL_SEL1_200H;
        RL_SEL2 = RL_SEL2_200H;
        RL_SEL3 = RL_SEL3_200H;
        
    }else if(freq == 150.0f){
        RL_SEL1 = RL_SEL1_150H;
        RL_SEL2 = RL_SEL2_150H;
        RL_SEL3 = RL_SEL3_150H;
        
    }else if(freq == 100.0f){
        RL_SEL1 = RL_SEL1_100H;
        RL_SEL2 = RL_SEL2_100H;
        RL_SEL3 = RL_SEL3_100H;
        
    }else if(freq == 75.0f){
        RL_SEL1 = RL_SEL1_75H;
        RL_SEL2 = RL_SEL2_75H;
        RL_SEL3 = RL_SEL3_75H;
        
    }else if(freq == 50.0f){
        RL_SEL1 = RL_SEL1_50H;
        RL_SEL2 = RL_SEL2_50H;
        RL_SEL3 = RL_SEL3_50H;
        
    }else if(freq == 30.0f){
        RL_SEL1 = RL_SEL1_30H;
        RL_SEL2 = RL_SEL2_30H;
        RL_SEL3 = RL_SEL3_30H;
        
    }else if(freq == 25.0f){
        RL_SEL1 = RL_SEL1_25H;
        RL_SEL2 = RL_SEL2_25H;
        RL_SEL3 = RL_SEL3_25H;
        
    }else if(freq == 20.0f){
        RL_SEL1 = RL_SEL1_20H;
        RL_SEL2 = RL_SEL2_20H;
        RL_SEL3 = RL_SEL3_20H;
        
    }else if(freq == 15.0f){
        RL_SEL1 = RL_SEL1_15H;
        RL_SEL2 = RL_SEL2_15H;
        RL_SEL3 = RL_SEL3_15H;
        
    }else if(freq == 10.0f){
        RL_SEL1 = RL_SEL1_10H;
        RL_SEL2 = RL_SEL2_10H;
        RL_SEL3 = RL_SEL3_10H;
        
    }else if(freq == 7.5f){
        RL_SEL1 = RL_SEL1_7_5H;
        RL_SEL2 = RL_SEL2_7_5H;
        RL_SEL3 = RL_SEL3_7_5H;
        
    }else if(freq == 5.0f){
        RL_SEL1 = RL_SEL1_5H;
        RL_SEL2 = RL_SEL2_5H;
        RL_SEL3 = RL_SEL3_5H;
        
    }else if(freq == 3.0f){
        RL_SEL1 = RL_SEL1_3H;
        RL_SEL2 = RL_SEL2_3H;
        RL_SEL3 = RL_SEL3_3H;
        
    }else if(freq == 2.5f){
        RL_SEL1 = RL_SEL1_2_5H;
        RL_SEL2 = RL_SEL2_2_5H;
        RL_SEL3 = RL_SEL3_2_5H;
        
    }else if(freq == 2.0f){
        RL_SEL1 = RL_SEL1_2H;
        RL_SEL2 = RL_SEL2_2H;
        RL_SEL3 = RL_SEL3_2H;
        
    }else if(freq == 1.5f){
        RL_SEL1 = RL_SEL1_1_5H;
        RL_SEL2 = RL_SEL2_1_5H;
        RL_SEL3 = RL_SEL3_1_5H;
        
    }else if(freq == 1.0f){
        RL_SEL1 = RL_SEL1_1H;
        RL_SEL2 = RL_SEL2_1H;
        RL_SEL3 = RL_SEL3_1H;
        
    }else if(freq == 0.75f){
        RL_SEL1 = RL_SEL1_0_75H;
        RL_SEL2 = RL_SEL2_0_75H;
        RL_SEL3 = RL_SEL3_0_75H;
        
    }else if(freq == 0.5f){
        RL_SEL1 = RL_SEL1_0_5H;
        RL_SEL2 = RL_SEL2_0_5H;
        RL_SEL3 = RL_SEL3_0_5H;
        
    }else if(freq == 0.3f){
        RL_SEL1 = RL_SEL1_0_3H;
        RL_SEL2 = RL_SEL2_0_3H;
        RL_SEL3 = RL_SEL3_0_3H;
        
    }else if(freq == 0.25f){
        RL_SEL1 = RL_SEL1_0_25H;
        RL_SEL2 = RL_SEL2_0_25H;
        RL_SEL3 = RL_SEL3_0_25H;
        
    }else if(freq == 0.1f){
        RL_SEL1 = RL_SEL1_0_1H;
        RL_SEL2 = RL_SEL2_0_1H;
        RL_SEL3 = RL_SEL3_0_1H;
        
    }else{
        perror("Error: high cutoff frequency Kilo.");
    }
    RL = ((uint16_t)RL_SEL3 << 13) |((uint16_t)RL_SEL2 << 7) | RL_SEL1;
    write_command(INTAN_config, (uint8_t)ADC_LOW_FREQ_B, RL);

}



void power_up_AC(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, ADC_INDIVIDUAL_AMP_POWER, 0xFFFF);
}

void A_or_B_cutoff_frequency(INTAN_config_struct* INTAN_config){
    uint8_t i;
    uint16_t amp_values = 0;
    for (i = NUM_CHANNELS; i > 0; i--){
        if(INTAN_config->amplifier_cutoff_frequency_A_B[i-1] == 'A'){
            amp_values = amp_values | (1<<(i-1));
        }
    }
    write_command(INTAN_config, AMP_LOW_CUTOFF_FREQ_SELECT, amp_values);
    INTAN_config->waiting_trigger = true;
}

void amp_fast_settle(INTAN_config_struct* INTAN_config){
    uint8_t i;
    uint16_t amp_values = 0;
    for (i = NUM_CHANNELS; i > 0; i--){
        if(INTAN_config->amplfier_reset[i-1]){
            amp_values = amp_values | (1<<(i-1));
        }
    }
    uint16_t reg_config_num = INTAN_config->max_size;
    INTAN_config->time_restriction[reg_config_num] = true;
    write_command(INTAN_config, AMPLIFIER_FAST_SETTLE, amp_values);
    INTAN_config->waiting_trigger = true;   
}


void amp_fast_settle_reset(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, AMPLIFIER_FAST_SETTLE, 0x0000);
    INTAN_config->waiting_trigger = true;   
}



// step selection as DAC configuration as a general thing for the register 35
typedef struct {
    uint16_t current_nA;  // Current step in nA
    uint8_t sel1;         // 7 bits
    uint8_t sel2;         // 6 bits
    uint8_t sel3;         // 2 bits
} StepConfig;


static const StepConfig step_table[] = {
    // step_sel   sel1       sel2       sel3
    {  /*10nA*/ 10,      SEL1_10nA,         SEL2_10nA,         SEL3_10nA },  // SEL1_10nA, SEL2_10nA, SEL3_10nA
    {  /*20nA*/ 20,      SEL1_20nA,         SEL2_20nA,         SEL3_20nA },  
    {  /*50nA*/ 50,      SEL1_50nA,         SEL2_50nA,         SEL3_50nA },
    {  /*100nA*/ 100,   SEL1_100nA,        SEL2_100nA,         SEL3_100nA },
    {  /*200nA*/ 200,   SEL1_200nA,        SEL2_200nA,         SEL3_200nA },
    {  /*500nA*/ 500,   SEL1_500nA,        SEL2_500nA,         SEL3_500nA },
    {  /*1uA*/ 1000,      SEL1_1uA,          SEL2_1uA,         SEL3_1uA },
    {  /*2uA*/ 2000,      SEL1_2uA,          SEL2_2uA,         SEL3_2uA },
    {  /*5uA*/ 5000,      SEL1_5uA,          SEL2_5uA,         SEL3_5uA },
    { /*10uA*/ 10000,    SEL1_10uA,         SEL2_10uA,         SEL3_10uA }
};


uint16_t step_sel_united(uint16_t step_sel) {
    unsigned int i;
    for (i = (sizeof(step_table) / sizeof(step_table[0])) - 1; i < sizeof(step_table) / sizeof(step_table[0]); --i) {
        if (step_table[i].current_nA == step_sel) {
            StepConfig cfg = step_table[i];
            uint16_t result = 0;
            result |= (cfg.sel3 & 0x03) << 13;
            result |= (cfg.sel2 & 0x3F) << 7;
            result |= (cfg.sel1 & 0x7F);
            return result;
        }
    }
    return 0; // Return 0 if step_sel is invalid
}


void stim_step_DAC_configuration(INTAN_config_struct* INTAN_config){
    // uint16_t step_DAC_selected;
    uint16_t step_DAC_selected = step_sel_united(INTAN_config->step_DAC);
    write_command(INTAN_config, STIM_STEP_SIZE, step_DAC_selected);

}



// step selection as DAC configuration as a general thing for the register 35
typedef struct {
    uint16_t current_nA;  // Current step in nA
    uint8_t PBIAS;         // 7 bits
    uint8_t NBIAS;         // 6 bits
} BiasVoltageConfig;

static const BiasVoltageConfig bias_voltage_table[] = {
    // BiasVoltage   PBIAS       NBIAS       
    {  /*10nA*/ 10,      PBIAS_10nA,         NBIAS_10nA},  // SEL1_10nA, SEL2_10nA, SEL3_10nA
    {  /*20nA*/ 20,      PBIAS_20nA,         NBIAS_20nA},  
    {  /*50nA*/ 50,      PBIAS_50nA,         NBIAS_50nA},
    {  /*100nA*/ 100,   PBIAS_100nA,        NBIAS_100nA},
    {  /*200nA*/ 200,   PBIAS_200nA,        NBIAS_200nA},
    {  /*500nA*/ 500,   PBIAS_500nA,        NBIAS_500nA},
    {  /*1uA*/ 1000,      PBIAS_1uA,          NBIAS_1uA},
    {  /*2uA*/ 2000,      PBIAS_2uA,          NBIAS_2uA},
    {  /*5uA*/ 5000,      PBIAS_5uA,          NBIAS_5uA},
    { /*10uA*/ 10000,    PBIAS_10uA,         NBIAS_10uA}
};

uint16_t vias_voltages_sel(uint16_t step_sel) {
    unsigned int i;
    for (i = (sizeof(bias_voltage_table) / sizeof(bias_voltage_table[0])) - 1; i < sizeof(bias_voltage_table) / sizeof(bias_voltage_table[0]); --i) {
        if (bias_voltage_table[i].current_nA == step_sel) {
            BiasVoltageConfig cfg = bias_voltage_table[i];
            uint16_t result = 0;
            result |= (cfg.PBIAS & 0x3F) << 4;
            result |= (cfg.NBIAS & 0x7F);
            return result;
        }
    }
    return 0; // Return 0 if step_sel is invalid
}


void stim_PNBIAS_configuration(INTAN_config_struct* INTAN_config){
    // uint16_t step_DAC_selected;
    uint16_t BIAS_selected = vias_voltages_sel(INTAN_config->step_DAC);
    write_command(INTAN_config, STIM_BIAS_VOLTAGE, BIAS_selected);

}

// Configuration of the current for each channel, channels start from 0 to 15
void stim_current_channel_configuration(INTAN_config_struct* INTAN_config, uint8_t channel, uint8_t neg_current_trim, uint8_t neg_current_mag, uint8_t pos_current_trim, uint8_t pos_current_mag){
    if (channel > 15){
        while(1){
            ON_INTAN_LED();
        }
    }
    uint8_t curr_reg_neg = channel + 64;
    uint8_t curr_reg_pos = channel + 96;
    uint16_t negative = unify_8bits(neg_current_trim, neg_current_mag);
    uint16_t positive = unify_8bits(pos_current_trim, pos_current_mag);
    write_command(INTAN_config, curr_reg_neg, negative);
    write_command(INTAN_config, curr_reg_pos, positive);
    if (!(INTAN_config->waiting_trigger)){
        INTAN_config->waiting_trigger = true;
    }
}

//Turn off all channels for the stimulation
void all_stim_channels_off(INTAN_config_struct* INTAN_config){
    int i;
    for (i= NUM_CHANNELS-1; i>=0; i--){
        INTAN_config->stimulation_on[i] = 0;
    } 
}

// Stimulation on or off for each channel
void stimulation_on(INTAN_config_struct* INTAN_config){
    uint8_t i;
    uint16_t stim_on = 0;
    for (i = NUM_CHANNELS; i > 0; i--){
        if(INTAN_config->stimulation_on[i-1]){
            stim_on = stim_on | (1<<(i-1));
        }
    }
    write_command(INTAN_config, STIM_ON, stim_on);
    INTAN_config->waiting_trigger = true;
}

// Selection of the stimulation polarity for each channel
void stimulation_polarity(INTAN_config_struct* INTAN_config){
    uint8_t i;
    uint16_t stim_on = 0;
    for (i = NUM_CHANNELS; i > 0; i--){
        if(INTAN_config->stimulation_pol[i-1] == 'P'){
            stim_on = stim_on | (1<<(i-1));
        }else if (INTAN_config->stimulation_pol[i-1] == 'N') {
        
        }else{
            perror("Not correct polarity selected");
            while(1);
        }
    }
    write_command(INTAN_config, STIM_POLARITY, stim_on);
    INTAN_config->waiting_trigger = true;
}

void stimulation_disable(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, STIM_ENABLE_A_reg, 0x0000);
    write_command(INTAN_config, STIM_ENABLE_B_reg, 0x0000);
}

void stimulation_enable(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, STIM_ENABLE_A_reg, STIM_ENABLE_A_VALUE);
    write_command(INTAN_config, STIM_ENABLE_B_reg, STIM_ENABLE_B_VALUE);
}


void ON_INTAN(INTAN_config_struct* INTAN_config){

    /*
            CONSTANT CURRENT STIMULATOR
    */
    stimulation_enable(INTAN_config);

    send_SPI_commands(INTAN_config);

    /*
            COMPLIANCE MONITOR
    */
    enable_U_flag(INTAN_config);
    clean_compliance_monitor(INTAN_config);
    send_SPI_commands(INTAN_config);
    disable_U_flag(INTAN_config);
    /*
            CONSTANT CURRENT STIMULATOR // positive polarity
    */
    stimulation_polarity(INTAN_config); 
    send_SPI_commands(INTAN_config);


    // stimulators on
    stimulation_on(INTAN_config);
    read_command(INTAN_config, 255, 'E');
    send_SPI_commands(INTAN_config);


    /*
            U AND M FLAGS
    */
    enable_U_flag(INTAN_config);
    enable_M_flag(INTAN_config);
    read_command(INTAN_config, 255, 'E');
    send_SPI_commands(INTAN_config);
    disable_U_flag(INTAN_config);
    disable_M_flag(INTAN_config);
}

void OFF_INTAN(INTAN_config_struct* INTAN_config){
    stimulation_disable(INTAN_config);
    send_SPI_commands(INTAN_config);
}

// Clean the compliance monitor register
void clean_compliance_monitor(INTAN_config_struct* INTAN_config){
    enable_M_flag(INTAN_config);
    read_command(INTAN_config, REGISTER_VALUE_TEST, 't');
    disable_M_flag(INTAN_config);    
}


// Check the compliance monitor to see if all the channels are in a correct charging position
void check_compliance_monitor(INTAN_config_struct* INTAN_config){
    read_command(INTAN_config, COMPLIANCE_MONITOR, 'M');
}

// Activate the charge recovery switch by connecting a channel to the stim_GND electrode witting in channel 46. 
// Channels are connected one by one.
void connect_channel_to_gnd(INTAN_config_struct* INTAN_config, uint8_t channel){
    if (channel > 15){
        ON_INTAN_LED();
          perror("Trying to connect a too high channel to gnd.");
        while(1);
    }
    write_command(INTAN_config, CHRG_RECOV_SWITCH, channel+1);
    INTAN_config->waiting_trigger = true;
}

void disconnect_channels_from_gnd(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, CHRG_RECOV_SWITCH, 0x0000);
    INTAN_config->waiting_trigger = true;
}

// Recovery current limited to get to the Vrecov
typedef struct {
    uint16_t current_nA;  // Current step in nA
    uint8_t sel1;         // 7 bits
    uint8_t sel2;         // 6 bits
    uint8_t sel3;         // 2 bits
} chrg_recov_curr_lim_config;

static const chrg_recov_curr_lim_config chrg_recov_curr_lim_table[] = {
    // step_sel   sel1       sel2       sel3
    {  /*1nA*/ 1,        CL_SEL1_1nA,         CL_SEL2_1nA,         CL_SEL3_1nA },  // SEL1_10nA, SEL2_10nA, SEL3_10nA
    {  /*2nA*/ 2,        CL_SEL1_2nA,         CL_SEL2_2nA,         CL_SEL3_2nA },  
    {  /*5nA*/ 5,        CL_SEL1_5nA,         CL_SEL2_5nA,         CL_SEL3_5nA },
    {  /*10nA*/ 10,     CL_SEL1_10nA,        CL_SEL2_10nA,         CL_SEL3_10nA },
    {  /*20nA*/ 20,     CL_SEL1_20nA,        CL_SEL2_20nA,         CL_SEL3_20nA },
    {  /*50nA*/ 50,     CL_SEL1_50nA,        CL_SEL2_50nA,         CL_SEL3_50nA },
    {  /*100nA*/ 100,  CL_SEL1_100nA,       CL_SEL2_100nA,         CL_SEL3_100nA },
    {  /*200nA*/ 200,  CL_SEL1_200nA,       CL_SEL2_200nA,         CL_SEL3_200nA },
    {  /*500nA*/ 500,  CL_SEL1_500nA,       CL_SEL2_500nA,         CL_SEL3_500nA },
    {  /*1000nA*/ 1000,  CL_SEL1_1uA,         CL_SEL2_1uA,         CL_SEL3_1uA }
};

uint16_t chrg_recov_curr_lim_united(uint16_t curr_lim_sel) {
    unsigned int i;
    for (i = (sizeof(chrg_recov_curr_lim_table) / sizeof(chrg_recov_curr_lim_table[0])) - 1; i < sizeof(chrg_recov_curr_lim_table) / sizeof(chrg_recov_curr_lim_table[0]); --i) {
        if (chrg_recov_curr_lim_table[i].current_nA == curr_lim_sel) {
            chrg_recov_curr_lim_config cfg = chrg_recov_curr_lim_table[i];
            uint16_t result = 0;
            result |= (cfg.sel3 & 0x03) << 13;
            result |= (cfg.sel2 & 0x3F) << 7;
            result |= (cfg.sel1 & 0x7F);
            return result;
        }
    }
    return 0; // Return 0 if curr_lim_sel is invalid
}

void charge_recovery_current_configuration(INTAN_config_struct* INTAN_config){
    uint16_t current_lim_sel = chrg_recov_curr_lim_united(INTAN_config->current_recovery);
    write_command(INTAN_config, CHARGE_RECOVERY_CURRENT_LIMIT, current_lim_sel);
}


// Voltage at witch the electrodes will be driven with a concrete current value
void charge_recovery_voltage_configuration(INTAN_config_struct* INTAN_config){
    double voltage = INTAN_config->voltage_recovery;
    // Force the value of the voltage to be between +1.225 and -1.215

    if (voltage < -1.225) voltage = -1.225;
    if (voltage >  1.215) voltage =  1.215;

    // Transformación: valor_reg = 128 + (V / 0.00957)
    int value = (int)round(128.0 + (voltage / 0.00957));

    // Make sure is between 0 and 255
    if (value < 0)   value = 0;
    if (value > 255) value = 255;

    write_command(INTAN_config, CURRENT_LIMITED_CHARGE_RECOVERY_VOLTAGE_TARGET, value);

}


void enable_charge_recovery_sw(INTAN_config_struct* INTAN_config, uint8_t channel){
    write_command(INTAN_config, CURR_LIM_CHRG_RECOV_EN, channel+1);
    INTAN_config->waiting_trigger = true;
}

void disable_charge_recovery_sw(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, CURR_LIM_CHRG_RECOV_EN, 0x0000);
    INTAN_config->waiting_trigger = true;
}

// ADC Buffer Bias Current related registers
void ADC_sampling_rate_config(INTAN_config_struct* INTAN_config){
    uint16_t sampling_rate = INTAN_config->ADC_sampling_rate;
    uint8_t ADC_buffer_bias;
    uint8_t MUX_bias;
        if (sampling_rate <= 120){
            ADC_buffer_bias = ADC_BUFFER_BIAS_less_120k;
            MUX_bias = MUX_BIAS_less_120k;
        } else if (sampling_rate > 120 & sampling_rate <= 140) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_140k;
            MUX_bias = MUX_BIAS_140k;
        } else if (sampling_rate > 140 & sampling_rate <= 175) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_175k;
            MUX_bias = MUX_BIAS_175k;        
        } else if (sampling_rate > 175 & sampling_rate <= 220) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_220k;
            MUX_bias = MUX_BIAS_220k;        
        } else if (sampling_rate > 220 & sampling_rate <= 280) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_280k;
            MUX_bias = MUX_BIAS_280k;        
        } else if (sampling_rate > 280 & sampling_rate <= 350) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_350k;
            MUX_bias = MUX_BIAS_350k;        
        } else if (sampling_rate > 350 & sampling_rate <= 440) {
            ADC_buffer_bias = ADC_BUFFER_BIAS_440k;
            MUX_bias = MUX_BIAS_440k;        
        } else {
            ADC_buffer_bias = ADC_BUFFER_BIAS_more_440k;
            MUX_bias = MUX_BIAS_more_440k;                
        }

    uint16_t bias_value = ((uint16_t)ADC_buffer_bias << 6) | MUX_bias;

    write_command(INTAN_config, ADC_BIAS_BUFFER, bias_value);
    
    
}


// Impedance check control register 2 configuration
void impedance_check_control(INTAN_config_struct* INTAN_config){
    uint8_t zcheck_select = INTAN_config->zcheck_select;
    bool zcheck_DAC_enhable = INTAN_config->zcheck_DAC_enhable;
    bool zcheck_load = INTAN_config->zcheck_load;
    uint8_t zcheck_scale = INTAN_config->zcheck_scale;
    bool zcheck_en = INTAN_config->zcheck_en;
    
    if (zcheck_select > 15){
        ON_INTAN_LED();
        while(1);
    }
    
    uint16_t reg_value = (((uint16_t)zcheck_select << 8) | ((uint16_t)zcheck_DAC_enhable << 6) | ((uint16_t)zcheck_load << 5) | ((uint16_t)zcheck_scale << 3) | ((uint16_t)zcheck_en));
    write_command(INTAN_config, IMPEDANCE_CHECK_CONTROL, reg_value);
}

// Impedance check DAC value
void impedance_check_DAC(INTAN_config_struct* INTAN_config){
    write_command(INTAN_config, IMPEDANCE_CHECK_DAC, INTAN_config->zcheck_DAC_value);

}



// Fault current detection
void fault_current_detection(INTAN_config_struct* INTAN_config){
    read_command(INTAN_config, FAULT_CURR_DET, 'f');
}


// Enabling and controlling the digital external outputs 
void enable_digital_output_1(INTAN_config_struct* INTAN_config){
    INTAN_config->digout1HZ = 0;
    write_register_1(INTAN_config);
}
void enable_digital_output_2(INTAN_config_struct* INTAN_config){
    INTAN_config->digout2HZ = 0;
    write_register_1(INTAN_config);
}

// Disabling digital output 1 and 2
void disable_digital_output_1(INTAN_config_struct* INTAN_config){
    INTAN_config->digout1HZ = 1;
    write_register_1(INTAN_config);
}
void disable_digital_output_2(INTAN_config_struct* INTAN_config){
    INTAN_config->digout2HZ = 1;
    write_register_1(INTAN_config);
}

// ON output 1 or 2
void power_ON_output_1(INTAN_config_struct* INTAN_config){
    INTAN_config->digout1 = 1;
    write_register_1(INTAN_config);
}
void power_ON_output_2(INTAN_config_struct* INTAN_config){
    INTAN_config->digout2 = 1;
    write_register_1(INTAN_config);
}

// OFF output 1 or 2
void power_OFF_output_1(INTAN_config_struct* INTAN_config){
    INTAN_config->digout1 = 0;
    write_register_1(INTAN_config);
}
void power_OFF_output_2(INTAN_config_struct* INTAN_config){
    INTAN_config->digout2 = 0;
    write_register_1(INTAN_config);
}

// Compliment 2 enable and disable 
void enable_C2(INTAN_config_struct* INTAN_config){
    INTAN_config->C2_enabled = 1;
    write_register_1(INTAN_config);
}
void disable_C2(INTAN_config_struct* INTAN_config){
    INTAN_config->C2_enabled = 0;    
    write_register_1(INTAN_config);
}

// Enabling and disabling absolute value configuration
void enable_absolute_value(INTAN_config_struct* INTAN_config){
    INTAN_config->abs_mode = true;    
    write_register_1(INTAN_config);
}

void disable_absolute_value(INTAN_config_struct* INTAN_config){
    INTAN_config->abs_mode = false;    
    write_register_1(INTAN_config);
}

// Enable or disable the digital signal processing HPF
void disable_digital_signal_processing_HPF(INTAN_config_struct* INTAN_config){
    INTAN_config->DSPen = true;
    write_register_1(INTAN_config);
}
void enable_digital_signal_processing_HPF(INTAN_config_struct* INTAN_config){
    INTAN_config->DSPen = false;
    write_register_1(INTAN_config);
}

// Digital signal processing filter HPF cutoff frequency configuration
void DSP_cutoff_frequency_configuration(INTAN_config_struct* INTAN_config){
    float_t f_sampling;
    f_sampling = INTAN_config->ADC_sampling_rate/INTAN_config->number_channels_to_convert; 
    float_t K;
    K = INTAN_config->DSP_cutoff_freq/f_sampling;
    if (K>=0.1103){
        INTAN_config->DSP_cutoff_freq_register = 1;

    }else if (K >= 0.04579 & K < 0.1103) {
        INTAN_config->DSP_cutoff_freq_register = 2;    
    
    }else if (K >= 0.02125 & K < 0.04579) {
        INTAN_config->DSP_cutoff_freq_register = 3;    
    
    }else if (K >= 0.01027 & K < 0.02125) {
        INTAN_config->DSP_cutoff_freq_register = 4;  
         
    }else if (K >= 0.005053 & K < 0.01027) {
        INTAN_config->DSP_cutoff_freq_register = 5;    
    
    }else if (K >= 0.002506 & K < 0.005053) {
        INTAN_config->DSP_cutoff_freq_register = 6;    
    
    }else if (K >= 0.001248 & K < 0.002506) {
        INTAN_config->DSP_cutoff_freq_register = 7;    
    
    }else if (K >= 0.0006229 & K < 0.001248) {
        INTAN_config->DSP_cutoff_freq_register = 8;    
    }else if (K >= 0.0003112 & K < 0.0006229) {
        INTAN_config->DSP_cutoff_freq_register = 9;    
    }else if (K >= 0.0001555 & K < 0.0003112) {
        INTAN_config->DSP_cutoff_freq_register = 10;    
    }else if (K >= 0.00007773 & K < 0.0001555) {
        INTAN_config->DSP_cutoff_freq_register = 11;    
    }else if (K >= 0.00003886 & K < 0.00007773) {
        INTAN_config->DSP_cutoff_freq_register = 12;    
    }else if (K >= 0.00001943 & K < 0.00003886) {
        INTAN_config->DSP_cutoff_freq_register = 13;    
    }else if (K >= 0.000009714 & K < 0.00001943) {
        INTAN_config->DSP_cutoff_freq_register = 14;    
    }else if (K >= 0.000004857 & K < 0.000009714) {
        INTAN_config->DSP_cutoff_freq_register = 15;    
    }else if (K < 0.000004857) {
        INTAN_config->DSP_cutoff_freq_register = 15;
    }
    write_register_1(INTAN_config);
}


// Write in register 1 the cutoff frequency of the digital signal processing HPF

void write_register_1(INTAN_config_struct* INTAN_config){
    uint16_t DATA = 
    ((uint16_t)INTAN_config->digoutOD   << 12) |
    ((uint16_t)INTAN_config->digout2    << 11) |
    ((uint16_t)INTAN_config->digout2HZ  << 10) |
    ((uint16_t)INTAN_config->digout1    << 9)  |
    ((uint16_t)INTAN_config->digout1HZ  << 8)  |
    ((uint16_t)INTAN_config->weak_MISO  << 7)  |
    ((uint16_t)INTAN_config->C2_enabled << 6)  |
    ((uint16_t)INTAN_config->abs_mode   << 5)  |
    ((uint16_t)INTAN_config->DSPen      << 4)  |
    ((uint16_t)INTAN_config->DSP_cutoff_freq_register);
    write_command(INTAN_config, REGISTER_1, DATA);
}

// Minimum power disipation writting FFFF in register 38: error in hardware produces that this register must be all ones.

void minimum_power_disipation(INTAN_config_struct* INTAN_config){

    write_command(INTAN_config, DC_AMPLIFIER_POWER, 0xFFFF);
    
    write_command(INTAN_config, CH0_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH1_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH2_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH3_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH4_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH5_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH6_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH7_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH8_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH9_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH10_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH11_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH12_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH13_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH14_NEG_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH15_NEG_CURR_MAG, 0x0000);
    
    write_command(INTAN_config, CH0_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH1_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH2_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH3_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH4_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH5_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH6_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH7_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH8_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH9_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH10_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH11_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH12_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH13_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH14_POS_CURR_MAG, 0x0000);
    write_command(INTAN_config, CH15_POS_CURR_MAG, 0x0000);

    // // Charge recovery current limit to 1 nA
    // INTAN_config->current_recovery = 1;
    // charge_recovery_current_configuration(INTAN_config);

    // // stimulation current step
    // INTAN_config->step_DAC = 10;
    // stim_step_DAC_configuration(INTAN_config);


    // // Second AC amplifier lower cutoff frequency
    // INTAN_config->fc_low_B = 0.1f;
    // fc_low_B(INTAN_config);

    // Power down un used AC amplifiers
    write_command(INTAN_config, ADC_INDIVIDUAL_AMP_POWER, 0x0000);

}


#include "../../common/INTAN_config.h"

#include "../../common/values_macro.h"
#include "../../common/register_macro.h"

#include <math.h>
#include <stdbool.h> 
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Necesario para uint16_t
#include "functions/general_functions.h"

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
    Function for initializing INTAN for the MSP430
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

    }
    INTAN_config->C2_enabled = false;

    INTAN_config->initial_channel_to_convert = 3;
    INTAN_config->step_DAC = 10;
    INTAN_config->waiting_trigger = false;
    
    INTAN_config->current_recovery = 20;
    INTAN_config->voltage_recovery = -1.225;
    
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
void send_SPI_commands(int state, INTAN_config_struct* INTAN_config, uint8_t* packet_1, uint8_t* packet_2, uint8_t* packet_3, uint8_t* packet_4){

    /*
    Se añade a la lista de envíos dos dummies para que se puedan comprobar todas las recepciones tras los envíos.
    Si no se hace esto entonces habría que implementar una lógica adicional de traslado de paquetes hacia el inicio de los que se han recibido y 
    comprobarlo más adelante, aciendo que el fallo venga más tarde de lo deseado y pudiendo producir un error indeseado como una configuración mala.
    */
    
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    reg_config_num += 1;

    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    reg_config_num += 1;
    
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;  

    uint16_t pckt_count = 0;
    uint8_t rx_packet_1 = 1;
    uint8_t rx_packet_2 = 2;
    uint8_t rx_packet_3 = 3;
    uint8_t rx_packet_4 = 4;

    while(pckt_count != INTAN_config->max_size){       
            
            wait_1_second();
            
            OFF_CS_pin();
            __delay_cycles(CLK_5_CYCLES);

            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = INTAN_config->array1[pckt_count];
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            rx_packet_1=UCA0RXBUF;
            __delay_cycles(CLK_2_CYCLES);

            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?
            UCA0TXBUF = INTAN_config->array2[pckt_count];
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            rx_packet_2=UCA0RXBUF;
            __delay_cycles(CLK_2_CYCLES);

            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?              
            UCA0TXBUF = INTAN_config->array3[pckt_count];
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            rx_packet_3=UCA0RXBUF;
            __delay_cycles(CLK_2_CYCLES);

            while (!(IFG2 & UCA0TXIFG));              // USART1 TX buffer ready?              
            UCA0TXBUF = INTAN_config->array4[pckt_count];
            while (!(IFG2 & UCA0RXIFG));  // espera que RXBUF tenga el dato recibido
            rx_packet_4=UCA0RXBUF;
            __delay_cycles(CLK_2_CYCLES);

            __delay_cycles(CLK_10_CYCLES);
            ON_CS_pin();   
            
            __delay_cycles(CLK_5_CYCLES);

            uint32_t rx_value = ((uint32_t)rx_packet_1 << 24) |
                ((uint32_t)rx_packet_2 << 16) |
                ((uint32_t)rx_packet_3 << 8)  |
                ((uint32_t)rx_packet_4);
            INTAN_config->obtained_RX[pckt_count] = rx_value;
            pckt_count += 1;
        
    }
    pckt_count = 0;
    
    bool checked_rcvd = check_received_commands(INTAN_config);
    if (!checked_rcvd){
        // while(1){
            ON_INTAN_LED();
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
    for (index = 0; index < INTAN_config->max_size-1; index++){
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
          
        }
    }

    return true;
}

//Function to check if the INTAN is working at all
void check_intan_SPI_array(INTAN_config_struct* INTAN_config){
    volatile uint16_t reg_config_num = INTAN_config->max_size;

    //1.  SPI ficticio tras encender para asegurar que el controlador esta en el estado correcto
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 1;
    INTAN_config->instruction[reg_config_num] = 'K';
    reg_config_num += 1;

    //1.  SPI ficticio tras encender para asegurar que el controlador esta en el estado correcto
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 1;
    INTAN_config->instruction[reg_config_num] = 'K';
    reg_config_num += 1;

    //1.  SPI ficticio tras encender para asegurar que el controlador esta en el estado correcto
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 1;
    INTAN_config->instruction[reg_config_num] = 'K';
    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;
}


void clear_command(INTAN_config_struct* INTAN_config){
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    // uint8_t obtained_RX_i = INTAN_config->obtained_RX_i;
    // Comando de limpieza recomendado en la pg 33 para inicializar el ADC para su operación normal
    INTAN_config->array1[reg_config_num] = CLEAR_ACTION;
    INTAN_config->array2[reg_config_num] = ZEROS_8;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;


    if (INTAN_config->C2_enabled == false){
        INTAN_config->expected_RX[reg_config_num] = ZEROS_32;
    } else{
        INTAN_config->expected_RX[reg_config_num] = RETURN_CLEAR_2C;
    }
    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    INTAN_config->instruction[reg_config_num] = 'L';


    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;

}

void write_command(INTAN_config_struct* INTAN_config, uint8_t R, uint16_t D){
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    // Flag U on M off
    if ((INTAN_config->U_flag == true)&&(INTAN_config->M_flag == false)){
        INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;

    // Flag U off M on
    } else if ((INTAN_config->U_flag == false)&&(INTAN_config->M_flag == true)) {
        INTAN_config->array1[reg_config_num] = WRITE_ACTION_M;
    
    // Flag U on M on
    }else if ((INTAN_config->U_flag == true)&&(INTAN_config->M_flag == true)) {
        INTAN_config->array1[reg_config_num] = WRITE_ACTION_U_M;
    
    // Flag U off M off
    }else{
        INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    }

    INTAN_config->array2[reg_config_num] = R;
    uint8_t high_byte;
    uint8_t low_byte;

    split_uint16(D, &high_byte, &low_byte);
    INTAN_config->array3[reg_config_num] = high_byte;
    INTAN_config->array4[reg_config_num] = low_byte;
    uint32_t returned; 
    returned = unify_16bits(0b1111111111111111, D);
    INTAN_config->expected_RX[reg_config_num] = returned;
    INTAN_config->expected_RX_bool[reg_config_num] = 1;
    INTAN_config->instruction[reg_config_num] = 'W';

    reg_config_num += 1;

    INTAN_config->max_size = reg_config_num;

}

void read_command(INTAN_config_struct* INTAN_config, uint8_t R, char id){
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    // Flag U on M off
    if ((INTAN_config->U_flag == true)&&(INTAN_config->M_flag == false)){
        INTAN_config->array1[reg_config_num] = READ_ACTION_U;

    // Flag U off M on
    } else if ((INTAN_config->U_flag == false)&&(INTAN_config->M_flag == true)) {
        INTAN_config->array1[reg_config_num] = READ_ACTION_M;
    
    // Flag U on M on
    }else if ((INTAN_config->U_flag == true)&&(INTAN_config->M_flag == true)) {
        INTAN_config->array1[reg_config_num] = READ_ACTION_U_M;
    
    // Flag U off M off
    }else{
        INTAN_config->array1[reg_config_num] = READ_ACTION;
    }

    INTAN_config->array2[reg_config_num] = R;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;

    INTAN_config->expected_RX[reg_config_num] = ZEROS_32;
    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    INTAN_config->instruction[reg_config_num] = id;

    reg_config_num += 1;

    INTAN_config->max_size = reg_config_num;
}



void fc_high(INTAN_config_struct* INTAN_config, char H_k, float_t freq){
    uint8_t RH1_sel1;
    uint8_t RH1_sel2;
    uint8_t RH2_sel1;
    uint8_t RH2_sel2;

    uint16_t RH1;
    uint16_t RH2;

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

void fc_low_A(INTAN_config_struct* INTAN_config, float_t freq){
    uint8_t RL_SEL1;
    uint8_t RL_SEL2;
    uint8_t RL_SEL3;

    uint16_t RL;
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

void fc_low_B(INTAN_config_struct* INTAN_config, float freq){
    uint8_t RL_SEL1;
    uint8_t RL_SEL2;
    uint8_t RL_SEL3;

    uint16_t RL;
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

void convert_channel(INTAN_config_struct* INTAN_config, uint8_t Channel){
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    volatile uint8_t saved_value;
    if (Channel >= 16){
        perror("Error: Too high Channel value.");
        while(1);
    }
    // No flags
    INTAN_config->array1[reg_config_num] = CONVERT_ACTION;
    
    // U flag
    if (INTAN_config->U_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_U; 
    } 
    
    // M flag
    if (INTAN_config->M_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_M; 
    } 
    
    // D flag 
    if (INTAN_config->D_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_D; 
    } 

    // H flag
    if (INTAN_config->H_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_H; 
    } 

    INTAN_config->array2[reg_config_num] = Channel;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;

    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    INTAN_config->instruction[reg_config_num] = 'O';
    INTAN_config->expected_RX[reg_config_num] = ZEROS_32;


    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;


}

void convert_N_channels(INTAN_config_struct* INTAN_config, uint8_t number_channels){
    volatile uint16_t reg_config_num = INTAN_config->max_size;
    volatile uint8_t saved_value; 
    volatile uint8_t Channel;
    uint8_t i;
    INTAN_config->max_size = reg_config_num;
    
    // Convert the first channel and then convert the N following channels
    Channel = INTAN_config->initial_channel_to_convert;
    if (Channel >= 16){
        perror("Error: Too high Channel value.");
        while(1);
    }

    // No flags
    INTAN_config->array1[reg_config_num] = CONVERT_ACTION;
    
    // U flag
    if (INTAN_config->U_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_U; 
    } 
    
    // M flag
    if (INTAN_config->M_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_M; 
    } 
    
    // D flag 
    if (INTAN_config->D_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_D; 
    } 

    // H flag
    if (INTAN_config->H_flag == true){
        saved_value = INTAN_config->array1[reg_config_num];
        INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_H; 
    } 

    INTAN_config->array2[reg_config_num] = Channel;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;

    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    INTAN_config->instruction[reg_config_num] = 'O';
    INTAN_config->expected_RX[reg_config_num] = ZEROS_32;


    reg_config_num += 1;

    for (i = number_channels; i>0; i--){
        // No flags
        INTAN_config->array1[reg_config_num] = CONVERT_ACTION;
        
        // U flag
        if (INTAN_config->U_flag == true){
            saved_value = INTAN_config->array1[reg_config_num];
            INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_U; 
        } 
        
        // M flag
        if (INTAN_config->M_flag == true){
            saved_value = INTAN_config->array1[reg_config_num];
            INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_M; 
        } 
        
        // D flag 
        if (INTAN_config->D_flag == true){
            saved_value = INTAN_config->array1[reg_config_num];
            INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_D; 
        } 

        // H flag
        if (INTAN_config->H_flag == true){
            saved_value = INTAN_config->array1[reg_config_num];
            INTAN_config->array1[reg_config_num] = saved_value | CONVERT_ACTION_H; 
        } 

        INTAN_config->array2[reg_config_num] = CONVERT_ACTION_N;
        INTAN_config->array3[reg_config_num] = ZEROS_8;
        INTAN_config->array4[reg_config_num] = ZEROS_8;

        INTAN_config->expected_RX_bool[reg_config_num] = 0;
        INTAN_config->expected_RX[reg_config_num] = ZEROS_32;
        INTAN_config->instruction[reg_config_num] = 'O';
    


        reg_config_num += 1;
    }
    INTAN_config->max_size = reg_config_num;
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
            result |= (cfg.PBIAS & 0x3F) << 8;
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



uint16_t unify_8bits(uint8_t high, uint8_t low) {
    return ((uint16_t)high << 8) | low;
}


uint32_t unify_16bits(uint16_t high, uint16_t low) {
    return ((uint32_t)high << 16) | low;
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

    // Make sure is beatween 0 and 255
    if (value < 0)   value = 0;
    if (value > 255) value = 255;

    write_command(INTAN_config, CURRENT_LIMITED_CHARGE_RECOVERY_VOLTAGE_TARGET, value);

}



































/**
 * Splits a 16-bit value into two 8-bit values.
 *
 * @param input     The 16-bit input value.
 * @param high_byte Pointer to store the high (most significant) 8 bits.
 * @param low_byte  Pointer to store the low (least significant) 8 bits.
 */
void split_uint16(uint16_t input, uint8_t* high_byte, uint8_t* low_byte) {
    if (high_byte) {
        *high_byte = (input >> 8) & 0xFF;  // Get the upper 8 bits
    }
    if (low_byte) {
        *low_byte = input & 0xFF;          // Get the lower 8 bits
    }
}

uint8_t calculate_current_lim_chr_recov(float target_voltage){
    if ((target_voltage > 1.225)||((target_voltage < -1.225))){
        return 0;
    }
    uint8_t dac_value = (uint8_t)roundf((target_voltage / 0.00957f) + 128.0f);
    return dac_value;
}

uint8_t calculate_stim_current(INTAN_config_struct* INTAN_config, uint16_t I_target_nA){
    uint8_t magnitude = (uint8_t)(I_target_nA / INTAN_config->step_sel);
    return magnitude;
}


void create_example_SPI_arrays(INTAN_config_struct* INTAN_config){
    uint8_t step_H;
    uint8_t step_L;
    split_uint16(step_sel_united(INTAN_config->step_sel), &step_H, &step_L);

    uint16_t BiasVol; 
    BiasVol = vias_voltages_sel(INTAN_config->step_sel);

    uint8_t dac_value;
    dac_value = calculate_current_lim_chr_recov(INTAN_config->target_voltage);

    uint8_t CL_H;
    uint8_t CL_L;
    split_uint16(chrg_recov_curr_lim_united(INTAN_config->CL_sel), &CL_H, &CL_L);



    uint16_t reg_config_num = 0;

    //1.  SPI ficticio tras encender para asegurar que el controlador esta en el estado correcto
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;
    
    // Testing -- it will be deleted
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = VALUES_VALUE_TEST_H;
    INTAN_config->array4[reg_config_num] = VALUES_VALUE_TEST_L;
    reg_config_num++;
    
    //REGISTER 32: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_A_reg;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //REGISTER 33: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_B_reg;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //REGISTER 34: Stimulation Current Step Size
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_STEP_SIZE;
    INTAN_config->array3[reg_config_num] = step_H;
    INTAN_config->array4[reg_config_num] = step_L;
    reg_config_num++;

    //REGISTER 35: Stimulation Bias Voltages
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_BIAS_VOLTAGE;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = BiasVol;
    reg_config_num++;

    //REGISTER 36: Current-Limited Charge Recovery Target Voltage
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CURRENT_LIMITED_CHARGE_RECOVERY_VOLTAGE_TARGET;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = dac_value;
    reg_config_num++;
    
    //REGISTER 37: Charge Recovery Current Limit
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CHARGE_RECOVERY_CURRENT_LIMIT;
    INTAN_config->array3[reg_config_num] = CL_H;
    INTAN_config->array4[reg_config_num] = CL_L;
    reg_config_num++;

    //REGISTER 38: Individual DC Amplifier Power
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = DC_AMPLIFIER_POWER;
    INTAN_config->array3[reg_config_num] = ONES_8;
    INTAN_config->array4[reg_config_num] = ONES_8;
    reg_config_num++;

    //REGISTER 40: Compliance Monitor (READ ONLY REGISTER WITH CLEAR)  - write action with clear
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_M;
    INTAN_config->array2[reg_config_num] = COMPLIANCE_MONITOR;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;


    //REGISTER 40: Compliance Monitor (READ ONLY REGISTER WITH CLEAR)  - read action
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = COMPLIANCE_MONITOR;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //REGISTER 40: Compliance Monitor (READ ONLY REGISTER WITH CLEAR)  - read action with clear
    INTAN_config->array1[reg_config_num] = READ_ACTION_M;
    INTAN_config->array2[reg_config_num] = COMPLIANCE_MONITOR;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //REGISTER 42: Stimulator On-Off (TRIGGERED REGISTER) - stimulation OFF
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = STIM_ON;
    INTAN_config->array3[reg_config_num] = ALL_CH_OFF;
    INTAN_config->array4[reg_config_num] = ALL_CH_OFF;
    reg_config_num++;

    //REGISTER 42: Stimulator On (TRIGGERED REGISTER) - stimulation ON in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = STIM_ON;
    INTAN_config->array3[reg_config_num] = CH_2_ON_L+CH_7_ON_L;
    INTAN_config->array4[reg_config_num] = CH_12_ON_H;
    reg_config_num++;


    //REGITER 44: Stimulator Polarity (TRIGGERED REGISTER) - stimulation positive in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = STIM_POLARITY;
    INTAN_config->array3[reg_config_num] = CH_2_ON_L+CH_7_ON_L;
    INTAN_config->array4[reg_config_num] = CH_12_ON_H;
    reg_config_num++;



    //REGISTER 46: Charge Recovery Switch (TRIGGERED REGISTER) - activated in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CHRG_RECOV_SWITCH;
    INTAN_config->array3[reg_config_num] = CH_2_ON_L+CH_7_ON_L;
    INTAN_config->array4[reg_config_num] = CH_12_ON_H;
    reg_config_num++;

    //REGISTER 48: Current-Limited Charge Recovery Enable (TRIGGERED REGISTER) - activated in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CURR_LIM_CHRG_RECOV_EN;
    INTAN_config->array3[reg_config_num] = CH_2_ON_L+CH_7_ON_L;
    INTAN_config->array4[reg_config_num] = CH_12_ON_H;
    reg_config_num++;

    //REGISTER 50:  Fault Current Detector (READ ONLY REGISTER) -
    // si se lee un valor de corriente peligroso, se podría realizar alguna acción que (por ejemplo) deshabilite la estimulación
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = FAULT_CURR_DET;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;



    uint8_t negative_current;
    negative_current = calculate_stim_current(INTAN_config, 1000);

    uint8_t trim_neg;
    trim_neg = 0;

    
    //REGISTER 64-79: Negative Stimulation Current Magnitude (TRIGGERED REGISTERS) - config ch 0 
    //Utiliza el valor de la configuración que hay en el step size,
    // si queremos utilizar un valor diferente habría que actualizarlo antes de utilizarlo
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CH0_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = trim_neg;
    INTAN_config->array4[reg_config_num] = negative_current;
    reg_config_num++;



    uint8_t positive_current;
    positive_current = calculate_stim_current(INTAN_config, 1000);

    uint8_t trim_pos;
    trim_pos= 0;
    
    //REGISTER 96-111: Positive Stimulation Current Magnitude (TRIGGERED REGISTERS)  - config ch 0 
    //Utiliza el valor de la configuración que hay en el step size,
    // si queremos utilizar un valor diferente habría que actualizarlo antes de utilizarlo
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CH0_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = trim_pos;
    INTAN_config->array4[reg_config_num] = positive_current;
    reg_config_num++;

    INTAN_config->max_size = reg_config_num;

}


void create_stim_SPI_arrays(INTAN_config_struct* INTAN_config){
    uint8_t step_H;
    uint8_t step_L;
    split_uint16(step_sel_united(INTAN_config->step_sel), &step_H, &step_L);

    uint16_t BiasVol; 
    BiasVol = vias_voltages_sel(INTAN_config->step_sel);

    uint8_t dac_value;
    dac_value = calculate_current_lim_chr_recov(INTAN_config->target_voltage);

    uint8_t CL_H;
    uint8_t CL_L;
    split_uint16(chrg_recov_curr_lim_united(INTAN_config->CL_sel), &CL_H, &CL_L);

    uint8_t CH_ON_selected_L = CH_2_ON_L+CH_7_ON_L;
    uint8_t CH_ON_selected_H = CH_12_ON_H;

    uint16_t reg_config_num = 0;
    
    //1.  SPI ficticio tras encender para asegurar que el controlador esta en el estado correcto
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //2.  Asegurar que la estimulación está deshabilitada
    //REGISTER 32: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_A_reg;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //REGISTER 33: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_B_reg;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    //3.  Encender todos los amplificadores de baja ganancia con acoplamiento DC para evitar un consumo excesivo de energía a causa de un error de HW
    // Si está a cero, se consumen 30.9mA adicionales de VDD
    //REGISTER 38: Individual DC Amplifier Power
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = DC_AMPLIFIER_POWER;
    INTAN_config->array3[reg_config_num] = ONES_8;
    INTAN_config->array4[reg_config_num] = ONES_8;
    reg_config_num++;

    // hay un monton de pasos entre medias relacionados con la parte de los ADC que vamos a ignorar... de momento
    
    //4.  Configurar el tamaño de paso de estimulacion (34)
    //REGISTER 34: Stimulation Current Step Size
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_STEP_SIZE;
    INTAN_config->array3[reg_config_num] = step_H;
    INTAN_config->array4[reg_config_num] = step_L;
    reg_config_num++;

    //5.  Establecer voltajes de polarizacion de estimulacion para el paso establecido en el registro 34 (35)
    //REGISTER 35: Stimulation Bias Voltages
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_BIAS_VOLTAGE;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = BiasVol;
    reg_config_num++;
    
    //6.  Establecer el voltaje objetivo de recuperacion de carga limitada en corriente (36)
    //REGISTER 36: Current-Limited Charge Recovery Target Voltage
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CURRENT_LIMITED_CHARGE_RECOVERY_VOLTAGE_TARGET;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = dac_value;
    reg_config_num++;    
    
    //7.  Establecer el límite de corriente de recuperación de carga (37)
    //REGISTER 37: Charge Recovery Current Limit
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = CHARGE_RECOVERY_CURRENT_LIMIT;
    INTAN_config->array3[reg_config_num] = CL_H;
    INTAN_config->array4[reg_config_num] = CL_L;
    reg_config_num++;

    //8.  Apagar todos los estimuladores (con U activa) (42)
    //REGISTER 42: Stimulator On-Off (TRIGGERED REGISTER) - stimulation OFF
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = STIM_ON;
    INTAN_config->array3[reg_config_num] = ALL_CH_OFF;
    INTAN_config->array4[reg_config_num] = ALL_CH_OFF;
    reg_config_num++;

    //9.  Establece todos los estimuladores en polaridad negativa (44)
    //REGITER 44: Stimulator Polarity (TRIGGERED REGISTER) - stimulation positive in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = STIM_POLARITY;
    INTAN_config->array3[reg_config_num] = CH_ON_selected_H;
    INTAN_config->array4[reg_config_num] = CH_ON_selected_L;
    reg_config_num++;

    //10. Abre todos los interruptores de recuperación de carga (46)
    //REGISTER 46: Charge Recovery Switch (TRIGGERED REGISTER) - activated in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CHRG_RECOV_SWITCH;
    INTAN_config->array3[reg_config_num] = CH_ON_selected_H;
    INTAN_config->array4[reg_config_num] = CH_ON_selected_L;
    reg_config_num++;

    //11. Abre todos los interruptores de recuperación de carga limitada en corriente (48)
    //REGISTER 48: Current-Limited Charge Recovery Enable (TRIGGERED REGISTER) - activated in 3 different registers
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CURR_LIM_CHRG_RECOV_EN;
    INTAN_config->array3[reg_config_num] = CH_ON_selected_H;
    INTAN_config->array4[reg_config_num] = CH_ON_selected_L;
    reg_config_num++;

    //12. Establece las corrientes negativas de estimulacion en cero centrando los ajustes de corriente (64...)
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH0_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;
    
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH1_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH2_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH3_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH4_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH5_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH6_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH7_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH8_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH9_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH10_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH11_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH12_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH13_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH14_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH15_NEG_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    //13. Establece las corrientes positivas de estimulacion en cero centrando los ajustes de corriente (96...)
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH0_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;
    
    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH1_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH2_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH3_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH4_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH5_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH6_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH7_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH8_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH9_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH10_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH11_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH12_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH13_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH14_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;

    INTAN_config->array1[reg_config_num] = WRITE_ACTION_U;
    INTAN_config->array2[reg_config_num] = CH15_POS_CURR_MAG;
    INTAN_config->array3[reg_config_num] = 0x80;
    INTAN_config->array4[reg_config_num] = 0x00;
    reg_config_num++;
    
    
    //14. Habilitar la estimulacion de los estimuladores (32, 33)
    //REGISTER 32: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_A_reg;
    INTAN_config->array3[reg_config_num] = STIM_ENABLE_A_VALUE_H;
    INTAN_config->array4[reg_config_num] = STIM_ENABLE_A_VALUE_L;
    reg_config_num++;

    //REGISTER 33: Enable - disable stimulation
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = STIM_ENABLE_B_reg;
    INTAN_config->array3[reg_config_num] = STIM_ENABLE_B_VALUE_H;
    INTAN_config->array4[reg_config_num] = STIM_ENABLE_B_VALUE_L;
    reg_config_num++;

    //15. Comando ficticio con flag M activado para limpiar el monitor de conformidad (255 - register value test)
    INTAN_config->array1[reg_config_num] = READ_ACTION_M;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    reg_config_num++;

    INTAN_config->max_size = reg_config_num;

}


void update_packets(uint16_t pckt_count, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4, INTAN_config_struct* INTAN_config) {
    
    // Retrieve values from each array at the given index
    *val1 = INTAN_config->array1[pckt_count];
    *val2 = INTAN_config->array2[pckt_count];
    *val3 = INTAN_config->array3[pckt_count];
    *val4 = INTAN_config->array4[pckt_count];
}

void wait_5_CYCLES(){
    __delay_cycles(CLK_5_CYCLES);
}

void wait_30_seconds(){
    __delay_cycles(CLK_30_S_CYCLES);
}

void wait_5_mins(){
    __delay_cycles(CLK_5_M_CYCLES);
}

void wait_1_second(){
    __delay_cycles(CLK_1_S_CYCLES);
}

void wait_3_seconds(){
    __delay_cycles(CLK_3_S_CYCLES);
}

void wait_5_seconds(){
    __delay_cycles(CLK_5_S_CYCLES);
}



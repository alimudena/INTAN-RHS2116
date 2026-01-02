#include "../../common/INTAN_config.h"

#include "../../common/values_macro.h"
#include "../../common/register_macro.h"

#include <math.h>
#include <stdbool.h> 
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Necesario para uint16_t
#include "functions/general_functions.h"



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



/*
    Function for sending and receiving the values through SPI with MSP430FG479
*/

void send_values(INTAN_config_struct* INTAN_config, uint16_t pckt_count){
    uint8_t rx_packet_1 = 1;
    uint8_t rx_packet_2 = 2;
    uint8_t rx_packet_3 = 3;
    uint8_t rx_packet_4 = 4;

    while (!(IFG2 & UCB0TXIFG));              // USART1 TX buffer ready?
    UCB0TXBUF = INTAN_config->array1[pckt_count];
    while (!(IFG2 & UCB0RXIFG));  // espera que RXBUF tenga el dato recibido
    rx_packet_1=UCB0RXBUF;
    // __delay_cycles(CLK_2_CYCLES);

    while (!(IFG2 & UCB0TXIFG));              // USART1 TX buffer ready?
    UCB0TXBUF = INTAN_config->array2[pckt_count];
    while (!(IFG2 & UCB0RXIFG));  // espera que RXBUF tenga el dato recibido
    rx_packet_2=UCB0RXBUF;
    // __delay_cycles(CLK_2_CYCLES);

    while (!(IFG2 & UCB0TXIFG));              // USART1 TX buffer ready?              
    UCB0TXBUF = INTAN_config->array3[pckt_count];
    while (!(IFG2 & UCB0RXIFG));  // espera que RXBUF tenga el dato recibido
    rx_packet_3=UCB0RXBUF;
    // __delay_cycles(CLK_2_CYCLES);

    while (!(IFG2 & UCB0TXIFG));              // USART1 TX buffer ready?              
    UCB0TXBUF = INTAN_config->array4[pckt_count];
    while (!(IFG2 & UCB0RXIFG));  // espera que RXBUF tenga el dato recibido
    rx_packet_4=UCB0RXBUF;
    // __delay_cycles(CLK_2_CYCLES);

    
    uint32_t rx_value = ((uint32_t)rx_packet_1 << 24) |
        ((uint32_t)rx_packet_2 << 16) |
        ((uint32_t)rx_packet_3 << 8)  |
        ((uint32_t)rx_packet_4);
    INTAN_config->obtained_RX[pckt_count] = rx_value;
}

/*
    Function for preparing the for confirmation protocol to be sent through SPI with MSP430FG479
*/


void send_confirmation_values(INTAN_config_struct* INTAN_config){
    uint16_t reg_config_num = INTAN_config->max_size;
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    reg_config_num += 1;

    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    reg_config_num += 1;
    
    INTAN_config->array1[reg_config_num] = READ_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;
    INTAN_config->expected_RX[reg_config_num] = CHIP_ID;
    INTAN_config->expected_RX_bool[reg_config_num] = 0;
    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;  
}



//Function to check if the INTAN is working at all SPECIFIC FUNCTION FOR THE MSP430FG479
void check_intan_SPI_array(INTAN_config_struct* INTAN_config){
    uint16_t reg_config_num = INTAN_config->max_size;

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

//Clear command sent to the INTAN through SPI by the MSP430FG479
void clear_command(INTAN_config_struct* INTAN_config){
    uint16_t reg_config_num = INTAN_config->max_size;
    // uint8_t obtained_RX_i = INTAN_config->obtained_RX_i;
    // Comando de limpieza recomendado en la pg 33 para inicializar el ADC para su operación normal
    INTAN_config->array1[reg_config_num] = CLEAR_ACTION;
    INTAN_config->array2[reg_config_num] = ZEROS_8;
    INTAN_config->array3[reg_config_num] = ZEROS_8;
    INTAN_config->array4[reg_config_num] = ZEROS_8;


    if (INTAN_config->C2_enabled == true){
        INTAN_config->expected_RX[reg_config_num] = ZEROS_32;
    } else{
        INTAN_config->expected_RX[reg_config_num] = RETURN_CLEAR_2C;
    }
    INTAN_config->expected_RX_bool[reg_config_num] = 1;
    INTAN_config->instruction[reg_config_num] = 'L';


    reg_config_num += 1;
 
    INTAN_config->max_size = reg_config_num;

}
//Write command sent to the INTAN through SPI by the MSP430FG479
void write_command(INTAN_config_struct* INTAN_config, uint8_t R, uint16_t D){
    uint16_t reg_config_num = INTAN_config->max_size;
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

//Read command sent to the INTAN through SPI by the MSP430FG479
void read_command(INTAN_config_struct* INTAN_config, uint8_t R, char id){
    uint16_t reg_config_num = INTAN_config->max_size;
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



// Convert channel command sent to the INTAN through SPI by the MSP430FG479
void convert_channel(INTAN_config_struct* INTAN_config, uint8_t Channel){
    uint16_t reg_config_num = INTAN_config->max_size;
    uint8_t saved_value;
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

// Convert N channels command sent to the INTAN through SPI by the MSP430FG479
void convert_N_channels(INTAN_config_struct* INTAN_config){
    uint16_t reg_config_num = INTAN_config->max_size;
    uint8_t saved_value; 
    uint8_t Channel;
    uint8_t i;
    INTAN_config->max_size = reg_config_num;
    uint8_t number_channels;
    uint8_t initial_channel;
    number_channels = INTAN_config->number_channels_to_convert;
    initial_channel = INTAN_config->initial_channel_to_convert;
    
    
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
    INTAN_config->max_size = reg_config_num;

    for (i = initial_channel; i<number_channels+initial_channel; i++){
        convert_channel(INTAN_config, i);
    }
    
}


// Convert N channels command sent to the INTAN through SPI by the MSP430FG479
void convert_N_channels_faster(INTAN_config_struct* INTAN_config){
    uint16_t reg_config_num = INTAN_config->max_size;
    uint8_t Channel;
    uint8_t i;
    INTAN_config->max_size = reg_config_num;
    uint8_t number_channels;
    uint8_t initial_channel;
    number_channels = INTAN_config->number_channels_to_convert;
    initial_channel = INTAN_config->initial_channel_to_convert;
    
    // Convert the first channel and then convert the N following channels
    Channel = INTAN_config->initial_channel_to_convert;
    if (Channel >= 16){
        perror("Error: Too high Channel value.");
        // while(1);
    }
    for (i = initial_channel; i<number_channels+initial_channel; i++){
        convert_channel(INTAN_config, i);
    }
    
}


uint8_t SPI_READ_BYTE(uint8_t TX){
        uint8_t RX;
        while (!(IFG2 & UCB0TXIFG));  /* espera TX listo */
        UCB0TXBUF = TX++;
        while (!(IFG2 & UCB0RXIFG));  /* espera dato RX */
        RX = UCB0RXBUF;
        return RX;
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



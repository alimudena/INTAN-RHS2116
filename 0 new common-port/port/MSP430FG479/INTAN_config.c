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



/**
 * @brief Send and receive a 4-byte packet over SPI and store the assembled 32-bit result.
 *
 * Sends four bytes from the configuration arrays at index @p pckt_count over SPI
 * and reads four bytes back from the SPI receive buffer. The received bytes are
 * combined into a 32-bit value and stored in @c INTAN_config->obtained_RX[pckt_count].
 *
 * @param INTAN_config Pointer to the INTAN configuration structure containing
 *                     transmit arrays and storage for received values.
 * @param pckt_count   Index of the packet within the arrays to send/receive.
 * @note This function blocks until each SPI TX/RX transfer completes.
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

/**
 * @brief Prepare confirmation read commands in the configuration arrays.
 *
 * Appends a few read actions into the INTAN configuration arrays that are used
 * to confirm communication with the INTAN device (expected responses set to
 * @c CHIP_ID). Updates @c INTAN_config->max_size accordingly.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
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



/**
 * @brief Populate configuration arrays with self-check read commands for INTAN.
 *
 * Adds several read commands into the configuration arrays that will be used
 * to verify the INTAN device is responding correctly after power-up. Each
 * entry uses @c REGISTER_VALUE_TEST and expects @c CHIP_ID. The function also
 * sets an instruction marker for each added entry and updates @c max_size.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 */
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

/**
 * @brief Append a CLEAR command to the configuration arrays.
 *
 * Adds the recommended clear command (per device documentation) to initialize
 * the ADC for normal operation. The expected response depends on whether C2
 * mode is enabled. Updates @c INTAN_config->max_size accordingly.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 */
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
/**
 * @brief Append a WRITE command to the configuration arrays.
 *
 * Prepares a write action for register @p R and 16-bit data @p D. The function
 * chooses the appropriate WRITE action byte depending on the U and M flags,
 * splits @p D into high and low bytes, computes the expected received value
 * (via @c unify_16bits), sets the instruction marker and increments
 * @c INTAN_config->max_size.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 * @param R            Register/address to write to.
 * @param D            16-bit data value to write.
 */
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

/**
 * @brief Append a READ command to the configuration arrays.
 *
 * Prepares a read action for register @p R. The function selects the proper
 * read action byte according to the U and M flags, sets zero data bytes and
 * expected response flags, records the instruction @p id, and increments
 * @c INTAN_config->max_size.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 * @param R            Register/address to read from.
 * @param id           Instruction identifier stored alongside the command.
 */
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



/**
 * @brief Append a single CONVERT channel command to the configuration arrays.
 *
 * Validates the @p Channel value, composes the CONVERT action byte with any
 * active U/M/D/H flags, and writes the channel and zero data bytes into the
 * arrays. The function sets instruction 'O', clears expected RX flag for this
 * entry and updates @c INTAN_config->max_size.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 * @param Channel      Channel number to convert (0..15). Function aborts on invalid channel.
 */
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

/**
 * @brief Append commands to convert N consecutive channels starting at initial.
 *
 * Uses @c INTAN_config->initial_channel_to_convert and
 * @c INTAN_config->number_channels_to_convert to schedule a convert for the
 * initial channel and then calls @c convert_channel for each following channel.
 * Validates channel bounds and updates @c INTAN_config->max_size.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 */
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


/**
 * @brief Faster version to schedule conversion for N channels.
 *
 * Iterates from @c initial_channel_to_convert for @c number_channels_to_convert
 * and calls @c convert_channel for each channel. This variant focuses on
 * scheduling conversions in a tight loop and does not add the initial
 * explicit CONVERT action entry itself before the loop.
 *
 * @param INTAN_config Pointer to the INTAN configuration structure to modify.
 */
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
        while(1);
    }
    for (i = initial_channel; i<number_channels+initial_channel; i++){
        convert_channel(INTAN_config, i);
    }
    
}


/**
 * @brief Transmit one byte via SPI and receive the response byte.
 *
 * Sends the provided transmit byte @p TX over the UCB0 SPI interface and
 * waits for the receive flag. Returns the received byte from the hardware
 * receive buffer.
 *
 * @param TX Byte to transmit over SPI.
 * @return Received byte from SPI.
 */
uint8_t SPI_READ_BYTE(uint8_t TX){
    uint8_t RX;
    while (!(IFG2 & UCB0TXIFG));  /* espera TX listo */
    UCB0TXBUF = TX++;
    while (!(IFG2 & UCB0RXIFG));  /* espera dato RX */
    RX = UCB0RXBUF;
    return RX;
}


/**
 * @brief Busy-wait for approximately 5 clock cycles.
 *
 * Wrapper around the vendor-provided @c __delay_cycles macro to delay the
 * CPU for a small fixed number of cycles defined by @c CLK_5_CYCLES.
 */
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



#include <stdint.h>  // Neccesary for  uint16_t

#ifndef INTAN_CONFIG_H
#define INTAN_CONFIG_H

typedef struct{
    uint16_t step_sel;
    uint8_t array1[8];
    uint8_t array2[8];
    uint8_t array3[8];
    uint8_t array4[8];
    uint32_t max_size;
    float target_voltage;
    } INTAN_config_struct;

#endif


/**
 * @brief Retrieves 4 values from internal arrays at a given packet index.
 * 
 * @param pckt_count Index in the arrays.
 * @param val1 Pointer to store the first value.
 * @param val2 Pointer to store the second value.
 * @param val3 Pointer to store the third value.
 * @param val4 Pointer to store the fourth value.
 */

void create_arrays(INTAN_config_struct* INTAN_config);

void update_packets(uint16_t pckt_count, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4, INTAN_config_struct INTAN_config);

uint16_t step_sel_united(uint16_t step_sel);

void split_uint16(uint16_t input, uint8_t* high_byte, uint8_t* low_byte);


uint8_t calculate_current_lim_chr_recov(float target_voltage);

#include <stdint.h>  // Neccesary for  uint16_t

#ifndef INTAN_CONFIG_H
#define INTAN_CONFIG_H

#define FREQ_MASTER 8000000U

#define CLK_30_S_CYCLES (30 * FREQ_MASTER)
#define CLK_3_S_CYCLES (3 * FREQ_MASTER)

#define FIVE_SECONDS 5
#define CLK_5_S_CYCLES (FIVE_SECONDS * FREQ_MASTER)

#define FIVE_MINUTES 5*60
#define CLK_5_M_CYCLES (FIVE_MINUTES * FREQ_MASTER)

typedef struct{
    uint16_t step_sel;
    uint16_t CL_sel;
    uint8_t array1[50];
    uint8_t array2[50];
    uint8_t array3[50];
    uint8_t array4[50];
    uint32_t max_size;
    float target_voltage;
    uint16_t I_pos_target_nA;
    uint16_t I_neg_target_nA;
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

void create_example_SPI_arrays(INTAN_config_struct* INTAN_config);
void create_stim_SPI_arrays(INTAN_config_struct* INTAN_config);


void update_packets(uint16_t pckt_count, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4, INTAN_config_struct INTAN_config);

uint16_t step_sel_united(uint16_t step_sel);

void split_uint16(uint16_t input, uint8_t* high_byte, uint8_t* low_byte);

uint8_t calculate_current_lim_chr_recov(float target_voltage);

uint8_t calculate_stim_current(INTAN_config_struct* INTAN_config, uint16_t I_target_nA);

void wait_30_seconds();
void wait_5_mins();

void wait_3_seconds();
void wait_5_seconds();

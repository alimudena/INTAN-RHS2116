#include <stdint.h>  // Neccesary for  uint16_t
#include "stdbool.h"
#include <math.h>
#ifndef INTAN_CONFIG_H
#define INTAN_CONFIG_H

#define FREQ_MASTER 8000000U

#define CLK_30_S_CYCLES (30 * FREQ_MASTER)
#define CLK_3_S_CYCLES (3 * FREQ_MASTER)
#define CLK_1_S_CYCLES (1 * FREQ_MASTER)

#define FIVE_SECONDS 5
#define CLK_5_S_CYCLES (FIVE_SECONDS * FREQ_MASTER)

#define FIVE_MINUTES 5*60
#define CLK_5_M_CYCLES (FIVE_MINUTES * FREQ_MASTER)

#define CLK_2_CYCLES 2
#define CLK_5_CYCLES 5
#define CLK_10_CYCLES 10
#define NUM_STIM_EXP 5

#define MAX_VALUES 50

/* Definition of the structure that contains all the information related to the:
- Arrays for comparison of the received value
    - Expected RX array
    - Received RX array
    - To be confirmed array
- Flag M
- Flag U
*/
typedef struct{
    uint16_t step_sel;
    uint16_t CL_sel;
    uint8_t array1[MAX_VALUES];
    uint8_t array2[MAX_VALUES];
    uint8_t array3[MAX_VALUES];
    uint8_t array4[MAX_VALUES];
    uint32_t max_size;
    float target_voltage;
    uint16_t I_pos_target_nA;
    uint16_t I_neg_target_nA;

    bool M_flag; //To enable or disable flag M
    bool U_flag; //To enable or disable flag U
    bool D_flag; //To enable the flag for sampling DC low-gain (see convert command configuration)
    bool H_flag; //To enable the flag for DSP removal (see convert command configuration)
    
    uint32_t expected_RX[MAX_VALUES];
    uint32_t obtained_RX[MAX_VALUES];
    bool     expected_RX_bool[MAX_VALUES]; 
    char     instruction[MAX_VALUES];
    
    // Enabled 2Complements or not
    bool C2_enabled;

    uint8_t initial_channel_to_convert; // The initial channel we want to start converting when using convert_n_channels
    
    } INTAN_config_struct;

#endif

// Function for initializying INTAN configuration from MSP460fg479 information
void initialize_INTAN(INTAN_config_struct* INTAN_config);

// Functions for enabling and disabling M and U flags
void enable_M_flag(INTAN_config_struct* INTAN_config);
void enable_U_flag(INTAN_config_struct* INTAN_config);
void enable_D_flag(INTAN_config_struct* INTAN_config);
void enable_H_flag(INTAN_config_struct* INTAN_config);

void disable_M_flag(INTAN_config_struct* INTAN_config);
void disable_U_flag(INTAN_config_struct* INTAN_config);
void disable_H_flag(INTAN_config_struct* INTAN_config);
void disable_D_flag(INTAN_config_struct* INTAN_config);

// Function for clearing command - setting ADC calibration
void clear_command(INTAN_config_struct* INTAN_config);

// Function for write command
void write_command(INTAN_config_struct* INTAN_config, uint8_t R, uint16_t D);
void read_command(INTAN_config_struct* INTAN_config, uint8_t R);

// Function for sending SPI commands
void send_SPI_commands(int state, INTAN_config_struct* INTAN_config, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4);

//Function for checking the SPI received commands
bool check_received_commands(INTAN_config_struct* INTAN_config);

// Functions for setting the high and low cutoff frequencies for the ADC
void fc_high(INTAN_config_struct* INTAN_config, char H_k, float_t freq);
void fc_low_A(INTAN_config_struct* INTAN_config, float_t freq);
void fc_low_B(INTAN_config_struct* INTAN_config, float_t freq);

// Functions for sampling one channel or N channels
void convert_channel(INTAN_config_struct* INTAN_config, uint8_t Channel);
void convert_N_channels(INTAN_config_struct* INTAN_config, uint8_t number_channels);


// Unify two values of 8 or 16 bits to one of 16 bits
uint16_t unify_8bits(uint8_t high, uint8_t low);
uint32_t unify_16bits(uint16_t high, uint16_t low);

void create_example_SPI_arrays(INTAN_config_struct* INTAN_config);
void create_stim_SPI_arrays(INTAN_config_struct* INTAN_config);
void check_intan_SPI_array(INTAN_config_struct* INTAN_config);

/**
 * @brief Retrieves 4 values from internal arrays at a given packet index.
 * 
 * @param pckt_count Index in the arrays.
 * @param val1 Pointer to store the first value.
 * @param val2 Pointer to store the second value.
 * @param val3 Pointer to store the third value.
 * @param val4 Pointer to store the fourth value.
 */
void update_packets(uint16_t pckt_count, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4, INTAN_config_struct* INTAN_config);

uint16_t step_sel_united(uint16_t step_sel);

void split_uint16(uint16_t input, uint8_t* high_byte, uint8_t* low_byte);

uint8_t calculate_current_lim_chr_recov(float target_voltage);

uint8_t calculate_stim_current(INTAN_config_struct* INTAN_config, uint16_t I_target_nA);

void wait_5_CYCLES();

void wait_30_seconds();
void wait_5_mins();

void wait_1_second();
void wait_3_seconds();
void wait_5_seconds();

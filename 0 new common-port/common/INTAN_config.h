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
#define CLK_50_CYCLES 50


#define NUM_CHANNELS 16

#define MAX_VALUES 45

/* Definition of the structure that contains all the information related to the:
- Arrays for comparison of the received value
    - Expected RX array
    - Received RX array
    - To be confirmed array
- Flag M
- Flag U
*/
typedef struct{


    /* Parameters that may be needed but not necessarily*/
    uint8_t number_of_stimulations;
    uint32_t resting_time;
    uint16_t stimulation_time;
    float stimulation_on_time;
    float stimulation_off_time;




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
    

    uint8_t initial_channel_to_convert; // The initial channel we want to start converting when using convert_n_channels

    uint8_t number_channels_to_convert; // The number of channels we want to convert when using convert_n_channels
    
    uint16_t step_DAC;
    uint16_t PBIAS_curr;
    uint16_t NBIAS_curr;

    bool waiting_trigger;

    double voltage_recovery;
    uint16_t current_recovery;

    // ADC sampling rate frequency
    uint16_t ADC_sampling_rate;

    // register 1
    bool digoutOD;
    bool digout2;
    bool digout2HZ;
    bool digout1;
    bool digout1HZ;
    bool weak_MISO;
    bool C2_enabled;     // Enabled 2Complements or not
    bool abs_mode;
    bool DSPen;
    uint8_t DSP_cutoff_freq_register;

    // Cutoff frequency wanted for the DSP 
    float_t DSP_cutoff_freq;

    // register 2
    uint8_t zcheck_select;
    bool zcheck_DAC_enhable;
    bool zcheck_load;
    bool zcheck_scale;
    bool zcheck_en;

    // register 3
    uint8_t zcheck_DAC_value;

    char fh_unit;
    float_t fh_magnitude;
    float_t fc_low_A;
    float_t fc_low_B;
    char amplifier_cutoff_frequency_A_B[NUM_CHANNELS];

    bool amplfier_reset[NUM_CHANNELS];
    uint32_t time_restriction[MAX_VALUES]; // when a register needs some time before it is returned to its original value

    bool stimulation_on[NUM_CHANNELS];
    char stimulation_pol[NUM_CHANNELS];
    uint8_t negative_current_trim[NUM_CHANNELS];
    uint8_t negative_current_magnitude[NUM_CHANNELS];
    uint8_t positive_current_trim[NUM_CHANNELS];
    uint8_t positive_current_magnitude[NUM_CHANNELS];

    

    } INTAN_config_struct;

#endif

/*
    AMPLIFIER BANDWIDTH
*/
// Functions for setting the high and low cutoff frequencies for the ADC
void fc_high(INTAN_config_struct* INTAN_config);
void fc_low_A(INTAN_config_struct* INTAN_config);
void fc_low_B(INTAN_config_struct* INTAN_config);
void power_up_AC(INTAN_config_struct* INTAN_config);

// function for selecting in each channel wich is having a low or high cutoff shifting
void A_or_B_cutoff_frequency(INTAN_config_struct* INTAN_config);

// Function for settling the amplifiers to a baseline value, necessary some time after changing this to its original value
void amp_fast_settle(INTAN_config_struct* INTAN_config);
void amp_fast_settle_reset(INTAN_config_struct* INTAN_config);

/*
    CONSTANT CURRENT STIMULATOR
*/
// stim Step DAC configuration
uint16_t step_sel_united(uint16_t step_sel);
void stim_step_DAC_configuration(INTAN_config_struct* INTAN_config);

// Stim Pbias and Nbias configuration
void stim_PNBIAS_configuration(INTAN_config_struct* INTAN_config);

// Stimulation current configuration for each channel separately, positive and negative trim and magnitude
void stim_current_channel_configuration(INTAN_config_struct* INTAN_config, uint8_t channel, uint8_t neg_current_trim, uint8_t neg_current_mag, uint8_t pos_current_trim, uint8_t pos_current_mag);

// Turn off all channels for the stimulation
void all_stim_channels_off(INTAN_config_struct* INTAN_config);

// Stimulation on or off for each channel
void stimulation_on(INTAN_config_struct* INTAN_config);

// Selection of the stimulation polarity for each channel
void stimulation_polarity(INTAN_config_struct* INTAN_config);

// Writting in the registers 32 and 33 values for enabling and disabling the stimulation
void stimulation_disable(INTAN_config_struct* INTAN_config);
void stimulation_enable(INTAN_config_struct* INTAN_config);


void ON_INTAN(INTAN_config_struct* INTAN_config);
void OFF_INTAN(INTAN_config_struct* INTAN_config);

/*
    COMPLIANCE MONITOR
*/
// Compliance monitor related (register 40)
void clean_compliance_monitor(INTAN_config_struct* INTAN_config);
void check_compliance_monitor(INTAN_config_struct* INTAN_config);

/*
    CHARGE RECOVERY SWITCH
*/
// Connect the electrode to the gnd reference electrode
void connect_channel_to_gnd(INTAN_config_struct* INTAN_config, uint8_t channel);
void disconnect_channels_from_gnd(INTAN_config_struct* INTAN_config);

/*
    CURRENT LIMITED CHARGE RECOVERY CIRCUIT
*/
// Current-limited charge recovery circuit voltage and current configuration
void charge_recovery_current_configuration(INTAN_config_struct* INTAN_config);
void charge_recovery_voltage_configuration(INTAN_config_struct* INTAN_config);
void enable_charge_recovery_sw(INTAN_config_struct* INTAN_config, uint8_t channel);
void disable_charge_recovery_sw(INTAN_config_struct* INTAN_config);


/*
    FAULT CURRENT DETECTION
*/
// Fault current detection
void fault_current_detection(INTAN_config_struct* INTAN_config);


/*
    AUXILIARY DIGITAL OUTPUTS
*/
// Enabling and controlling the digital external outputs 
void enable_digital_output_1(INTAN_config_struct* INTAN_config);
void enable_digital_output_2(INTAN_config_struct* INTAN_config);
void disable_digital_output_1(INTAN_config_struct* INTAN_config);
void disable_digital_output_2(INTAN_config_struct* INTAN_config);


void power_ON_output_1(INTAN_config_struct* INTAN_config);
void power_ON_output_2(INTAN_config_struct* INTAN_config);
void power_OFF_output_1(INTAN_config_struct* INTAN_config);
void power_OFF_output_2(INTAN_config_struct* INTAN_config);

/*
    ABSOLUTE VALUE MODE
*/
// Enable and disable absolute values
void enable_absolute_value(INTAN_config_struct* INTAN_config);
void disable_absolute_value(INTAN_config_struct* INTAN_config);

/*
    DSP FOR HIGH PASS FILTER REMOVAL
*/
// Digital signal processing filter HPF enable or disable
void disable_digital_signal_processing_HPF(INTAN_config_struct* INTAN_config);
void enable_digital_signal_processing_HPF(INTAN_config_struct* INTAN_config);

// Digital signal processing filter HPF cutoff frequency configuration
void DSP_cutoff_frequency_configuration(INTAN_config_struct* INTAN_config);

/*
    POWER DISIPATION
*/
// Minimum power disipation writting FFFF in register 38: error in hardware produces that this register must be all ones.
void minimum_power_disipation(INTAN_config_struct* INTAN_config);

/*
    SPI COMMAND WORDS
*/
// Function for sending SPI commands
void send_SPI_commands(INTAN_config_struct* INTAN_config);

//Function for checking the SPI received commands
bool check_received_commands(INTAN_config_struct* INTAN_config);

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


// Functions for sampling one channel or N channels
void convert_channel(INTAN_config_struct* INTAN_config, uint8_t Channel);
void convert_N_channels(INTAN_config_struct* INTAN_config);

// Function for updating the stimulation parameters that come from an external device
void new_stimulation_parameters(INTAN_config_struct* INTAN_config);

// Function for clearing command - setting ADC calibration
void clear_command(INTAN_config_struct* INTAN_config);

// Function for write command
void write_command(INTAN_config_struct* INTAN_config, uint8_t R, uint16_t D);
void read_command(INTAN_config_struct* INTAN_config, uint8_t R, char id);

void send_values(INTAN_config_struct* INTAN_config, uint16_t pckt_count);
void send_confirmation_values(INTAN_config_struct* INTAN_config);

/*
    GENEREAL
*/
// Enables and disables C2 compliment dynamically if needed
void enable_C2(INTAN_config_struct* INTAN_config);
void disable_C2(INTAN_config_struct* INTAN_config);

// Sends a general SPI array for checking if the INTAN is correctly working 
void check_intan_SPI_array(INTAN_config_struct* INTAN_config);

// Configures many things
void write_register_1(INTAN_config_struct* INTAN_config);

/*
    ELECTRODE IMPEDANCE TEST
*/
// Impedance check control register 2 configuration
void impedance_check_control(INTAN_config_struct* INTAN_config);

// Impedance check DAC value
void impedance_check_DAC(INTAN_config_struct* INTAN_config);


/*
    ANALOG TO DIGITAL CONVERTER
*/
// Configuration of the ADC sampling rate, depending on the master's frequency of functioning
void ADC_sampling_rate_config(INTAN_config_struct* INTAN_config);
















// Unify two values of 8 or 16 bits to one of 16 bits
uint16_t unify_8bits(uint8_t high, uint8_t low);
uint32_t unify_16bits(uint16_t high, uint16_t low);








void split_uint16(uint16_t input, uint8_t* high_byte, uint8_t* low_byte);


void wait_5_CYCLES();

void wait_30_seconds();
void wait_5_mins();

void wait_1_second();
void wait_3_seconds();
void wait_5_seconds();

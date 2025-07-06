#include "../../common/INTAN_config.h"

#include "../../common/values_macro.h"
#include "../../common/register_macro.h"

#include <math.h>

// Step selection values as lookup tables
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

// Step selection values as lookup tables
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

// Step selection values as lookup tables
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


uint8_t vias_voltages_sel(uint16_t step_sel) {
    unsigned int i;
    for (i = (sizeof(bias_voltage_table) / sizeof(bias_voltage_table[0])) - 1; i < sizeof(bias_voltage_table) / sizeof(bias_voltage_table[0]); --i) {
        if (bias_voltage_table[i].current_nA == step_sel) {
            BiasVoltageConfig cfg = bias_voltage_table[i];
            uint8_t result = 0;
            result |= (cfg.PBIAS & 0x3F) << 4;
            result |= (cfg.NBIAS & 0x7F);
            return result;
        }
    }
    return 0; // Return 0 if step_sel is invalid
}


uint16_t chrg_recov_curr_lim_united(uint16_t curr_lim_sel) {
    unsigned int i;
    for (i = (sizeof(chrg_recov_curr_lim_table) / sizeof(chrg_recov_curr_lim_table[0])) - 1; i < sizeof(chrg_recov_curr_lim_table) / sizeof(chrg_recov_curr_lim_table[0]); --i) {
        if (chrg_recov_curr_lim_table[i].current_nA == curr_lim_sel) {
            StepConfig cfg = chrg_recov_curr_lim_table[i];
            uint16_t result = 0;
            result |= (cfg.sel3 & 0x03) << 13;
            result |= (cfg.sel2 & 0x3F) << 7;
            result |= (cfg.sel1 & 0x7F);
            return result;
        }
    }
    return 0; // Return 0 if curr_lim_sel is invalid
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
    uint8_t magnitude = (uint8_t)(I_neg_target_nA / INTAN_config->step_sel);
    return magnitude;
}

void create_arrays(INTAN_config_struct* INTAN_config){
    uint8_t step_H;
    uint8_t step_L;
    split_uint16(step_sel_united(INTAN_config->step_sel), &step_H, &step_L);

    uint8_t BiasVol; 
    BiasVol = vias_voltages_sel(INTAN_config->step_sel);

    uint8_t dac_value;
    dac_value = calculate_current_lim_chr_recov(INTAN_config->target_voltage);

    uint8_t CL_H;
    uint8_t CL_L;
    split_uint16(CL_sel_united(INTAN_config->CL_sel), &CL_H, &CL_L);



    uint16_t reg_config_num = 0;
    
    // Testing -- it will be deleted
    INTAN_config->array1[reg_config_num] = WRITE_ACTION;
    INTAN_config->array2[reg_config_num] = REGISTER_VALUE_TEST;
    INTAN_config->array3[reg_config_num] = VALUES_VALUE_TEST_H;
    INTAN_config->array4[reg_config_num] = VALUES_VALUE_TEST_L;
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
    INTAN_config->array2[reg_config_num] = CURRENT_LIMITED_CHARGE_RECOVERY;
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
    INTAN_config->array3[reg_config_num] = trim;
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

}

void update_packets(uint16_t pckt_count, uint8_t* val1, uint8_t* val2, uint8_t* val3, uint8_t* val4, INTAN_config_struct INTAN_config) {
    
    //Check that the index is within bounds

    if (pckt_count >= INTAN_config.max_size) {
        // If index is out of bounds, return 0s or error defaults
        *val1 = *val2 = *val3 = *val4 = 0;
        return;
    }

    // Retrieve values from each array at the given index
    *val1 = INTAN_config.array1[pckt_count];
    *val2 = INTAN_config.array2[pckt_count];
    *val3 = INTAN_config.array3[pckt_count];
    *val4 = INTAN_config.array4[pckt_count];
}






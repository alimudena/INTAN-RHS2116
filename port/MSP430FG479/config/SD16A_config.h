#include <stdbool.h> 

#ifndef SD16A_CONFIG_H
#define SD16A_CONFIG_H
typedef struct{
        // -- Entrada analógica
    int analog_input[5]; //0: A0, 1: A1, 2: A2, 3: A3, 4: A4
    int analog_input_ID[5]; //0: A0, 1: A1, 2: A2, 3: A3, 4: A4
    int analog_input_count;
    unsigned int analog_input_being_sampled;
        // -- Tensión de referencia
    char v_ref;            // I: Internal (1.2V), O: Off-chip, E: External
        // -- Reloj de referencia
    char clk_ref;          // M: MCLK, S: SMCLK, A: ACLK, T: TACLK
        // -- Divisor de frecuencia de referencia
    int clk_div_1;
    int clk_div_2;
        // -- Método de lectura: Polling o Interrupciones
    bool interruption_SD16A;
        // -- Over Sampling Ratio
    int OSR; //1, 32, 64, 128, 256, 512, 1024
        // -- Ganancia
    int gain; //1, 2, 4, 8, 16 or 32
        // -- Método de conversión
    char conv_mode; // C: Continuous  S: Single
        // -- Tipo de datos
    char polarity;       // B : Bipolar, U : unipolar
    char sign;           // O : Offset, C : 2's complement
    bool sampled;
    

    } SD16A_config_struct;


#endif

void setup_SD16A(SD16A_config_struct* SD16A_configuration);

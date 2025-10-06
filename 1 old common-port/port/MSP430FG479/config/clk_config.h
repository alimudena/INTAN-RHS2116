#include <stdbool.h> 

#ifndef CLK_CONFIG_H
#define CLK_CONFIG_H
typedef struct{
    char operating_mode; //Modo de operación
    char LFXT1_wk_mode; //Oscilador LFXT1
    int LFXT1_int_cap; //Configura la capacidad interna del LFXT1
    int DCO_range; //Rango de frecuencia de trabajo del DCO:
    bool DCOPLUS_on; // Values for setting the frequency of the DCO+; set so freq= xtal x D x N_MCLK+1 
    int D_val; //Max 8
    int N_MCLK; //Max 127
    char ref_MCLK; //Reference selection for MCLK
    char ref_SMCLK; // Reference for SMCLK
    int divider_ACLK; //ACLK division for configuring ACLK/N
    bool LFXT2_osc_on; //Second oscillator ON OFF
    bool CLK_debug; //Debug mode for clk activation or deactivation
    } CLK_config_struct;

#endif

void general_setup(CLK_config_struct CLK_config);
void setup_CLK(CLK_config_struct CLK_config);

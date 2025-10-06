#include <msp430.h>
#include <stdbool.h> 



//*****************************************************************************
/*CONFIGURATION FOR DEBUGGING/N*/
//*****************************************************************************
void configure_PINS_for_clk_debug();

void configure_MCLK_for_debug();


//*****************************************************************************
/*CONFIGURATION FOR ACLK/N*/
//*****************************************************************************

void configure_ACLK_N(int divider);

//*****************************************************************************
/*CONFIGURATION FOR MCLK*/
//*****************************************************************************
void select_reference_MCLK(char clk_ref_MCLK);

//void turn_off_MCLK(bool MCLK_OFF);
void configure_N_for_MCLK(int N_MCLK);

//*****************************************************************************
/*CONFIGURATION FOR SMCLK*/
//*****************************************************************************

void select_reference_SMCLK(char clk_ref_SMCLK);


//*****************************************************************************
/*CONFIGURATION FOR LFXT1 OSCILLATOR*/
//*****************************************************************************

void protection_cpu_required(bool osc_ext, int f_osc_ext);
void LFXT1_internal_cap_config(int LFXT1_int_cap);
void LFXT1_working_mode(char Low_High_power_mode);
void LFXT1_disabled();

//*****************************************************************************
/*CONFIGURATION FOR XT2 OSCILLATOR*/
//*****************************************************************************

void LFXT2_disable(bool LFXT2_disabled);

//*****************************************************************************
/*CONFIGURATION FOR DCO*/
//*****************************************************************************

void configuring_DCO(bool DCOPLUS_on, int D_val);

void DCO_f_range(int DCO_range);

//*****************************************************************************
/*BASIC CONFIGURATIONS PRESETS*/
//*****************************************************************************

void initClockTo8MHz();


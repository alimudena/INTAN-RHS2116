
#include <msp430.h>
#include <stdbool.h> 

//*****************************************************************************
/*SELECTING AND READING THE CHANNEL*/
//*****************************************************************************

void setup_analog_input(int channel);
void select_analog_input(int channel);

int read_analog_input();

//*****************************************************************************
/*CONFIGURING THE REFERENCES*/
//*****************************************************************************

void voltage_reference(char v_reference);

void gain_setup(int gain); 

void SD16_clk_reference(char clk_ref);
void fM_dividers(int div_1, int div_2);

void config_OSR(int OSR);
//*****************************************************************************
/*SELECTION OF THE CONVERSION MODE*/
//*****************************************************************************

void conversion_mode(char conv_mode);
void start_conversion(void);
void stop_conversion(void);

//*****************************************************************************
/*OUTPUT DATA FORMAT*/
//*****************************************************************************
void data_format(char polarity, char sign);


//*****************************************************************************
/*ENTERING LOW POWER MODE*/
//*****************************************************************************

void enter_LPM();

//void exit_LPM();


//*****************************************************************************
/*INTERRUPTION CONFIGURATION*/
//*****************************************************************************


void enable_interruption_SD16A(bool enable);

int IFG_polling(void);

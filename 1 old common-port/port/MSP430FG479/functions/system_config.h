#include "msp430fg479.h"
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>  // Neccesary for  uint16_t
#include <assert.h>  // For using assert
#include <stdbool.h> 

//*****************************************************************************
/*OPERATING MODES*/
//*****************************************************************************

// Operating modes selection
void select_operating_mode(char mode, int LPM_int);
// Switch off the LPM
void exit_LMP();


//*****************************************************************************
/*INTERRUPTIONS*/
//*****************************************************************************

//Enabling Interruptions
void enable_interruptions(bool enable);


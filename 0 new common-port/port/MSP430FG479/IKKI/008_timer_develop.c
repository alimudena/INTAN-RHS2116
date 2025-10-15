//  MSP430x47x Demo - Timer_A, Toggle P4.6, CCR0 Cont. Mode ISR, DCO SMCLK
//
//  Description: Toggle P4.6 using software and TA_0 ISR. Toggles every
//  50000 SMCLK cycles. SMCLK provides clock source for TACLK.
//  During the TA_0 ISR, P4.6 is toggled and 50000 clock cycles are added to
//  CCR0. TA_0 ISR is triggered every 50000 cycles. CPU is normally off and
//  used only during TA_ISR.
//  ACLK = n/a, MCLK = SMCLK = TACLK = default DCO
//
//           MSP430x47x
//         ---------------
//     /|\|            XIN|-
//      | |               |
//      --|RST        XOUT|-
//        |               |
//        |           P4.6|-->LED
//
//  M.Seamen/ P. Thanigai
//  Texas Instruments Inc.
//  September 2008
//  Built with IAR Embedded Workbench V4.11A and CCE V3.2
//*****************************************************************************
#include <msp430.h>
#include <stdbool.h> 
#include <stdint.h>  


#include "../config/clk_config.h"
#include "../config/TIMER_config.h"
#include "../functions/general_functions.h"
#include "../functions/TIMER.h"
#include "../functions/system_config.h"

#include "../../../common/INTAN_config.h"
#include "../../../common/INTAN_functions.h"
#include "../../../common/register_macro.h"


CLK_config_struct CLK_config;
TIMER_config_struct TIMER_config;
INTAN_config_struct INTAN_config;

uint32_t timing_counter_during_stimulation = 0;
uint32_t timing_counter_ON_OFF = 0;
bool stimulation_enabled_bool = false;
bool timing_control_bool = false;
bool state_changed = false; 
bool ON_OFF = true;
uint8_t number_of_stimulations_done = 0;



uint32_t off_timing = 122;
uint32_t on_timing = 400;


uint8_t state = 0;


int main(void)
{
  general_setup(CLK_config);
  setup_CLK(CLK_config);
  toggle_setup();
  // enable_interruptions(true);
  __bis_SR_register(GIE);
  enable_interrupt_TIMER();

  INTAN_config.number_of_stimulations = 5;



  /* TIMER SETUP */
  source_clock_select_TIMER('S');
  operating_mode_TIMER('U');



  TACCR0 = 0xffff;


  __bis_SR_register(LPM0_bits + GIE); // entra a modo bajo consumo y deja habilitadas interrupciones
     

}

// Timer A0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMERA0_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{

  if(ON_OFF){
    if(timing_counter_ON_OFF >= on_timing){
      timing_counter_ON_OFF = 0;
      ON_OFF = false;
    }
    timing_counter_ON_OFF ++;

    if(timing_counter_during_stimulation >= 122){
      timing_counter_during_stimulation = 0;
      switch (state) {
            case 0:
              state = 1;P4OUT ^= 0x40;
              break;
            case 1:
              state = 2;P4OUT ^= 0x40;
              break;
            case 2:
              state = 3;P4OUT ^= 0x40;
              break;
            case 3:
              state = 0;P4OUT ^= 0x40;
              break;
            default:
              perror("Error: not corret state.");
              break;  
        }
    }
    timing_counter_during_stimulation++;

  }else{
    state = 0;
    if(timing_counter_ON_OFF >= off_timing){
      timing_counter_ON_OFF = 0;
      ON_OFF = true;
    }
    timing_counter_ON_OFF ++;
  }


}


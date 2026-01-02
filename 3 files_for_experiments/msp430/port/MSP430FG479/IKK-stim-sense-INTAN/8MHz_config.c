#include <msp430.h>
#include "../functions/FLL.h"
#include "../config/clk_config.h"

//                |             P1.1|--> MCLK = 8Mhz  --> 57 (referencia DCO)
//                |             P1.4|--> SMCLK = 8MHz --> 54 (referencia DCO)
//                |             P1.5|--> ACLK = 32kHz --> 51
CLK_config_struct CLK_config;

void config_CLK_8MHz(CLK_config_struct CLK_config){
  stop_wd();
  CLK_config.DCO_range = 4;
  CLK_config.N_MCLK = 121;
  CLK_config.DCOPLUS_on = true;
  CLK_config.D_val = 1;
  DCO_f_range(CLK_config.DCO_range);
  configure_N_for_MCLK (CLK_config.N_MCLK);
  // configuring_DCO(CLK_config.DCOPLUS_on, CLK_config.D_val);
  LFXT1_internal_cap_config(18);
  FLL_CTL0 |= DCOPLUS;           // DCO+ set so freq= xtal x D x N+1  
  
}

int main(void)
{
  // WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  // const int DCO_range = 4;
  // DCO_f_range(DCO_range);
  // //SCFI0 |= FN_4;                            // x2 DCO freq, 8MHz nominal DCO
  // const int N_MCLK = 121;
  // configure_N_for_MCLK (N_MCLK);
  // //SCFQCTL = 121;                            // (121+1) x 32768 x 2 = 7.99 Mhz
  
  
  // FLL_CTL0 |= DCOPLUS + XCAP18PF;           // DCO+ set so freq= xtal x D x N+1
  
  
//  P1DIR = 0x22;                             // P1.1,5 to output direction
//  P1SEL = 0x22;                             // P1.1,5 to output MCLK & ACLK
//  P1SEL2 |= 0x02;
  config_CLK_8MHz(CLK_config);
  configure_PINS_for_clk_debug();

  while(1);                                 // Loop in place
}

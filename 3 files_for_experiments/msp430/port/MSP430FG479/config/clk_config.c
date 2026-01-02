#include "clk_config.h"
#include "../functions/general_functions.h"
#include "../functions/SD16_A.h"
#include "../functions/system_config.h"
#include "../functions/FLL.h"

void general_setup(CLK_config_struct CLK_config) {

  //*****************************************************************************
  /*GENERAL SETUP*/
  //*****************************************************************************

  stop_wd();
  CLK_config.CLK_debug = false;
  if (CLK_config.CLK_debug) {
    configure_PINS_for_clk_debug();
    // setup pin for led for toggle
    toggle_setup();
  }
}




void setup_CLK(CLK_config_struct CLK_config) {
  //*****************************************************************************
  /*SETUP CLK*/
  /*For generating the 8MHz:
      CLK_config.operating_mode = 'A';
      CLK_config.LFXT1_wk_mode = 'L';
      CLK_config.DCO_range = 4;
      CLK_config.DCOPLUS_on = true; //If D factor is wanted to be applied then
     -> True CLK_config.D_val = 2; //Max 8 CLK_config.N_MCLK = 121; //Max 127
      CLK_config.ref_MCLK = 'D'; //  D: DCO, X: XT2, A: LFXT1
      CLK_config.ref_SMCLK = 'D'; // D: DCO, X: XT2, N: OFF
      CLK_config.divider_ACLK = 1; // 1, 2, 4, 8
      CLK_config.LFXT2_osc_on = false;
  */
  //*****************************************************************************

  // Modo de operación
  /*    mode:
          - A: Active
          - L: Low Power
              - 0: LPM0
              - 1: LPM1
              - 2: LPM2
              - 3: LPM3
  */
  CLK_config.operating_mode = 'A';
  select_operating_mode(CLK_config.operating_mode, 0);
  // Oscilador LFXT1
  /*
      L: Low Frequency Mode --> f auxiliar de 323kHz conectado
      H: High Frequency Mode
  */
  CLK_config.LFXT1_wk_mode = 'L';
  // Configura la capacidad interna del LFXT1
  /*0  --> XIN Cap = XOUT Cap = 0pf */
  /*10 --> XIN Cap = XOUT Cap = 10pf */
  /*14 --> XIN Cap = XOUT Cap = 14pf */
  /*18 --> XIN Cap = XOUT Cap = 18pf */
  CLK_config.LFXT1_int_cap = 18;
  LFXT1_working_mode(CLK_config.LFXT1_wk_mode);
  LFXT1_internal_cap_config(CLK_config.LFXT1_int_cap);
  // DCO
  // Rango de frecuencia de trabajo del DCO:
  /*2 --> fDCOCLK =   1.4-12MHz*/
  /*3 --> fDCOCLK =   2.2-17Mhz*/
  /*4 --> fDCOCLK =   3.2-25Mhz*/ //-> 8 MHz
  /*8 --> fDCOCLK =     5-40Mhz*/
  CLK_config.DCO_range = 4;
  DCO_f_range(CLK_config.DCO_range);
  // Values for setting the frequency of the DCO+
  // DCO+ set so freq= xtal x D x N_MCLK+1
  // XTAL --> 32767Hz
  CLK_config.DCOPLUS_on =
      true;                // If D factor is wanted to be applied then -> True
  CLK_config.D_val = 2;    // Max 8
  CLK_config.N_MCLK = 121; // Max 127
  // CLK_config.N_MCLK = 95; // Max 127
  configuring_DCO(CLK_config.DCOPLUS_on, CLK_config.D_val);
  configure_N_for_MCLK(CLK_config.N_MCLK);
  // MCLK
  // Reference selection for MCLK
  CLK_config.ref_MCLK = 'D'; //  D: DCO, X: XT2, A: LFXT1
  select_reference_MCLK(CLK_config.ref_MCLK);
  // SMCLK
  //  Reference for SMCLK
  CLK_config.ref_SMCLK = 'D'; // D: DCO, X: XT2, N: OFF
  select_reference_SMCLK(CLK_config.ref_SMCLK);
  // ACLK
  // ACLK division for configuring ACLK/N
  CLK_config.divider_ACLK = 1; // 1, 2, 4, 8
  configure_ACLK_N(CLK_config.divider_ACLK);
  // LFXT2
  // Second oscillator ON OFF
  CLK_config.LFXT2_osc_on = false;
  LFXT2_disable(CLK_config.LFXT2_osc_on);
}


void run_functions_setup_CLK(CLK_config_struct CLK_config) {

  select_operating_mode(CLK_config.operating_mode, 0);


  LFXT1_working_mode(CLK_config.LFXT1_wk_mode);
  LFXT1_internal_cap_config(CLK_config.LFXT1_int_cap);

  DCO_f_range(CLK_config.DCO_range);

  configuring_DCO(CLK_config.DCOPLUS_on, CLK_config.D_val);
  configure_N_for_MCLK(CLK_config.N_MCLK);

  select_reference_MCLK(CLK_config.ref_MCLK);

  select_reference_SMCLK(CLK_config.ref_SMCLK);

  configure_ACLK_N(CLK_config.divider_ACLK);

  LFXT2_disable(CLK_config.LFXT2_osc_on);
}

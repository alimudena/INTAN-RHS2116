#include "SD16A_config.h"
#include "../IKKI/IKKI_MAC.h"
#include "../functions/SD16_A.h"

int i;
unsigned counter;

void setup_SD16A(SD16A_config_struct* SD16A_configuration) {
  for (counter = SD16A_configuration->analog_input_count; counter > 0; counter--) {
    setup_analog_input(SD16A_configuration->analog_input[counter - 1]);
  }
  select_analog_input(
      SD16A_configuration->analog_input[SD16A_configuration->analog_input_being_sampled]);
  FLL_CTL0 |= XCAP14PF; // Configure load caps
  for (i = 10000; i > 0; i--); // Delay for 32 kHz crystal to

  voltage_reference(SD16A_configuration->v_ref);
  SD16_clk_reference(SD16A_configuration->clk_ref); // M: MCLK, S: SMCLK, A: ACLK, T:TACLK
  fM_dividers(SD16A_configuration->clk_div_1, SD16A_configuration->clk_div_2);
  for (i = 10000; i > 0; i--); // Delay for 1.2V ref startup
  config_OSR(SD16A_configuration->OSR);
  gain_setup(SD16A_configuration->gain);
  conversion_mode(SD16A_configuration->conv_mode);
  data_format(SD16A_configuration->polarity, SD16A_configuration->sign);
}

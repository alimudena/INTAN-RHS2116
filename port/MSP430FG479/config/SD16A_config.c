#include "SD16A_config.h"
#include "../IKKI/IKKI_MAC.h"
#include "../functions/SD16_A.h"

int i;
unsigned counter;

void setup_SD16A(SD16A_config_struct SD16A_configuration) {

  // -- Entrada analógica
  SD16A_configuration.analog_input_count = 5;
  SD16A_configuration.analog_input_being_sampled = 0;

  SD16A_configuration.analog_input[0] = EEG1; // 3: A3 - EEG1
  SD16A_configuration.analog_input[1] = EEG2; // 2: A2 - EEG2
  SD16A_configuration.analog_input[2] = EEG3; // 1: A1 - EEG3
  SD16A_configuration.analog_input[3] = ECG;  // 0: A0 - ECG
  SD16A_configuration.analog_input[4] = BATT; // 4: A4 - BATT

  SD16A_configuration.analog_input_ID[0] = 0x30; // 3: A3 - EEG1
  SD16A_configuration.analog_input_ID[1] = 0x31; // 2: A2 - EEG2
  SD16A_configuration.analog_input_ID[2] = 0x32; // 1: A1 - EEG3
  SD16A_configuration.analog_input_ID[3] = 0x33; // 0: A0 - ECG
  SD16A_configuration.analog_input_ID[4] = 0x34; // 4: A4 - BATT

  // -- Tensión de referencia
  SD16A_configuration.v_ref =
      'I'; // I: Internal (1.2V), O: Off-chip, E: External
           // -- Reloj de referencia
  SD16A_configuration.clk_ref = 'M'; // M: MCLK, S: SMCLK, A: AC1LK, T: TACLK
                                     // -- Divisor de frecuencia de referencia
  SD16A_configuration.clk_div_1 = 1;
  SD16A_configuration.clk_div_2 = 1;
  // -- Método de lectura: Polling o Interrupciones
  SD16A_configuration.interruption_SD16A = true;
  // -- Over Sampling Ratio
  SD16A_configuration.OSR = 512; // 1, 32, 64, 128, 256, 512, 1024
  // -- Ganancia
  SD16A_configuration.gain = 1; // 1, 2, 4, 8, 16 or 32
  // -- Método de conversión
  SD16A_configuration.conv_mode = 'C'; // C: Continuous  S: Single
                                       // -- Tipo de datos
  SD16A_configuration.polarity = 'B';  // B : Bipolar, U : unipolar
  SD16A_configuration.sign = 'O';      // O : Offset, C : 2's complement

  SD16A_configuration.sampled = false;

  for (counter = SD16A_configuration.analog_input_count; counter > 0;
       counter--) {
    setup_analog_input(SD16A_configuration.analog_input[counter - 1]);
  }
  select_analog_input(
      SD16A_configuration
          .analog_input[SD16A_configuration.analog_input_being_sampled]);
  FLL_CTL0 |= XCAP14PF; // Configure load caps
  for (i = 10000; i > 0; i--)
    ; // Delay for 32 kHz crystal to

  voltage_reference(SD16A_configuration.v_ref);
  SD16_clk_reference(
      SD16A_configuration.clk_ref); // M: MCLK, S: SMCLK, A: ACLK, T:TACLK
  fM_dividers(SD16A_configuration.clk_div_1, SD16A_configuration.clk_div_2);
  for (i = 10000; i > 0; i--)
    ; // Delay for 1.2V ref startup
  config_OSR(SD16A_configuration.OSR);
  gain_setup(SD16A_configuration.gain);
  conversion_mode(SD16A_configuration.conv_mode);
  data_format(SD16A_configuration.polarity, SD16A_configuration.sign);
}

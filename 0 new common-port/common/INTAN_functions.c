#include "INTAN_functions.h"

void call_configuration_functions(INTAN_config_struct* INTAN_config){
                read_command(INTAN_config, 255, '1');
             
                send_SPI_commands(INTAN_config);
             

                /* 
                        STIMULATION DISABLE AND MINIMUM POWER DISIPATION
                */
                
                stimulation_disable(INTAN_config); // 0x0000 0x0000
                send_SPI_commands(INTAN_config);
                minimum_power_disipation(INTAN_config);
                clear_command(INTAN_config);
                ADC_sampling_rate_config(INTAN_config);

                send_SPI_commands(INTAN_config);
             
                /*
                        AUXILIARY DIGITAL OUTPUTS
                */
                disable_digital_output_1(INTAN_config);
                disable_digital_output_2(INTAN_config);
                power_OFF_output_1(INTAN_config);
                power_OFF_output_2(INTAN_config);
                /*
                        ABSOLUTE VALUE MODE
                */
                disable_absolute_value(INTAN_config);
                /*
                        DSP FOR HIGH PASS FILTER REMOVAL
                */
                disable_digital_signal_processing_HPF(INTAN_config);
                DSP_cutoff_frequency_configuration(INTAN_config);
                /*
                        GENERAL
                */
                disable_C2(INTAN_config);

                send_SPI_commands(INTAN_config);

                /*
                        ELECTRODE IMPEDANCE TEST
                */
                impedance_check_control(INTAN_config);
                impedance_check_DAC(INTAN_config);

                send_SPI_commands(INTAN_config);

                /*
                        AMPLIFIER BANDWIDTH
                */

                fc_high(INTAN_config);
                fc_low_A(INTAN_config);
                fc_low_B(INTAN_config);
                amp_fast_settle_reset(INTAN_config);          
                A_or_B_cutoff_frequency(INTAN_config);

                send_SPI_commands(INTAN_config);

             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                stim_step_DAC_configuration(INTAN_config);
                stim_PNBIAS_configuration(INTAN_config);    

                send_SPI_commands(INTAN_config);

                /*
                        CURRENT LIMITED CHARGE RECOVERY CIRCUIT
                */             
                charge_recovery_voltage_configuration(INTAN_config);
                charge_recovery_current_configuration(INTAN_config);
                send_SPI_commands(INTAN_config);
             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                all_stim_channels_off(INTAN_config);
                stimulation_on(INTAN_config);
                stimulation_polarity(INTAN_config);
                /*
                        CHARGE RECOVERY SWITCH
                */
                disconnect_channels_from_gnd(INTAN_config);
                /*
                        CURRENT LIMITED CHARGE RECOVERY CIRCUIT
                */
                disable_charge_recovery_sw(INTAN_config);
                send_SPI_commands(INTAN_config);
             
                /*
                        CONSTANT CURRENT STIMULATOR
                */
                int i;
                for (i = NUM_CHANNELS-1; i>=0; i--){
                        stim_current_channel_configuration(INTAN_config, i, INTAN_config->negative_current_trim[i],INTAN_config->negative_current_magnitude[i], INTAN_config->positive_current_trim[i], INTAN_config->positive_current_magnitude[i]);
                }

                send_SPI_commands(INTAN_config);


                send_SPI_commands(INTAN_config);


                /*
                        U AND M FLAGS
                */
                enable_U_flag(INTAN_config);
                enable_M_flag(INTAN_config);
                read_command(INTAN_config, 255, 'E');
                disable_U_flag(INTAN_config);
                disable_M_flag(INTAN_config);

                send_SPI_commands(INTAN_config);
}


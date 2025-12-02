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

        /*
                2s COMPLEMENT
        */
        if (INTAN_config->C2_enabled){
                enable_C2(INTAN_config);
        }else{
                disable_C2(INTAN_config);
        }

        /*
                ABSOLUTE VALUE MODE
        */

        if (INTAN_config->abs_mode){
                enable_absolute_value(INTAN_config);
        }else{
                disable_absolute_value(INTAN_config);
        }

        /*
                DSP FOR HIGH PASS FILTER REMOVAL
        */
        if (INTAN_config->DSPen){
                enable_digital_signal_processing_HPF(INTAN_config);
                DSP_cutoff_frequency_configuration(INTAN_config);
        }else{
                disable_digital_signal_processing_HPF(INTAN_config);
        }


        send_SPI_commands(INTAN_config);

        /*
                AUXILIARY DIGITAL OUTPUTS
        */
        disable_digital_output_1(INTAN_config);
        disable_digital_output_2(INTAN_config);
        power_OFF_output_1(INTAN_config);
        power_OFF_output_2(INTAN_config);


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

        enable_U_flag(INTAN_config);
        all_stim_channels_off(INTAN_config);
        stimulation_on(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        stimulation_polarity(INTAN_config);
        disable_U_flag(INTAN_config);


        /*
                CHARGE RECOVERY SWITCH
        */
        enable_U_flag(INTAN_config);
        disconnect_channels_from_gnd(INTAN_config);
        disable_U_flag(INTAN_config);

        /*
        CURRENT LIMITED CHARGE RECOVERY CIRCUIT
        */
        enable_U_flag(INTAN_config);
        disable_charge_recovery_sw(INTAN_config);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        /*
                CONSTANT CURRENT STIMULATOR
        */
        enable_U_flag(INTAN_config);
        int i;
        for (i = NUM_CHANNELS-1; i>=0; i--){
                stim_current_channel_configuration(INTAN_config, i, INTAN_config->negative_current_trim[i],INTAN_config->negative_current_magnitude[i], INTAN_config->positive_current_trim[i], INTAN_config->positive_current_magnitude[i]);
        }

        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);
        enable_M_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        clean_compliance_monitor(INTAN_config);
        send_SPI_commands(INTAN_config);
        disable_U_flag(INTAN_config);
        disable_M_flag(INTAN_config);
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


void call_sense_configuration_functions(INTAN_config_struct* INTAN_config, uint8_t channel){
        power_up_AC(INTAN_config);
        send_SPI_commands(INTAN_config);

        ADC_sampling_rate_config(INTAN_config);
        send_SPI_commands(INTAN_config);

        fc_high(INTAN_config);
        send_SPI_commands(INTAN_config);

        fc_low_A(INTAN_config);
        send_SPI_commands(INTAN_config);

        A_or_B_cutoff_frequency(INTAN_config);
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

        INTAN_config->amplfier_reset[channel] = 1;
        amp_fast_settle(INTAN_config);
        send_SPI_commands(INTAN_config);

        INTAN_config->amplfier_reset[channel] = 0;
        
        amp_fast_settle_reset(INTAN_config);
        send_SPI_commands(INTAN_config);

        disable_absolute_value(INTAN_config);
        send_SPI_commands(INTAN_config);

        disable_digital_signal_processing_HPF(INTAN_config);
        send_SPI_commands(INTAN_config);

        /*
                U AND M FLAGS
        */
        enable_U_flag(INTAN_config);
        enable_M_flag(INTAN_config);
        read_command(INTAN_config, 255, 'E');
        disable_U_flag(INTAN_config);
        disable_M_flag(INTAN_config);

}

void call_initialization_procedure_example(INTAN_config_struct* INTAN_config){

        read_command(INTAN_config, 255, '1');

        write_command(INTAN_config, 32, 0x0000);
        write_command(INTAN_config, 33, 0x0000);
        write_command(INTAN_config, 38, 0xFFFF);

        clear_command(INTAN_config);

        send_SPI_commands(INTAN_config);

        // write_command(INTAN_config, 0, 0x00C5);
        write_command(INTAN_config, 0, 0x0828); // REGISTRO 0 SAMPLING RATE: 14kHz
        write_command(INTAN_config, 1, 0x051A);
        write_command(INTAN_config, 2, 0x0040);
        write_command(INTAN_config, 3, 0x0080);
        write_command(INTAN_config, 4, 0x0016);
        write_command(INTAN_config, 5, 0x0017);
        write_command(INTAN_config, 6, 0x00A8);
        write_command(INTAN_config, 7, 0x000A);
        write_command(INTAN_config, 8, 0xFFFF);

        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 10, 0x0000);
        disable_U_flag(INTAN_config);

        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 12, 0xFFFF);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        write_command(INTAN_config, 34, 0x00E2);
        write_command(INTAN_config, 35, 0x00AA);
        write_command(INTAN_config, 36, 0x0080);
        write_command(INTAN_config, 37, 0x4F00);

        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 42, 0x0000);
        disable_U_flag(INTAN_config);


        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 44, 0x0000);
        disable_U_flag(INTAN_config);


        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 46, 0x0000);
        disable_U_flag(INTAN_config);


        enable_U_flag(INTAN_config);
        write_command(INTAN_config, 48, 0x0000);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        int i;
        for (i = 64; i<70; i++){
                write_command(INTAN_config, i, 0x8000);
        }

        send_SPI_commands(INTAN_config);

        for (i = 96; i<112; i++){
                write_command(INTAN_config, i, 0x8000);
        }
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        // write_command(INTAN_config, 32, 0xAAAA); // do not enable stimulation
        // write_command(INTAN_config, 33, 0x00FF); // do not enable stimulation

        // send_SPI_commands(INTAN_config);
        
        enable_M_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        read_command(INTAN_config, 255, 'e');
        disable_M_flag(INTAN_config);
        disable_U_flag(INTAN_config);
        send_SPI_commands(INTAN_config);

}

void call_initialization_procedure_example_test_INTAN_functions(INTAN_config_struct* INTAN_config){

        read_command(INTAN_config, 255, '1');
        minimum_power_disipation(INTAN_config);
        stimulation_disable(INTAN_config);
        clear_command(INTAN_config);
        send_SPI_commands(INTAN_config);

        ADC_sampling_rate_config(INTAN_config);
        disable_C2(INTAN_config);
        // enable_C2(INTAN_config);
        disable_absolute_value(INTAN_config);
        // enable_absolute_value(INTAN_config);
        enable_digital_signal_processing_HPF(INTAN_config);
        // disable_digital_signal_processing_HPF(INTAN_config);
        DSP_cutoff_frequency_configuration(INTAN_config);

        disable_digital_output_1(INTAN_config);
        disable_digital_output_2(INTAN_config);
        power_OFF_output_1(INTAN_config);
        power_OFF_output_2(INTAN_config);

        impedance_check_control(INTAN_config);
        impedance_check_DAC(INTAN_config);

        
        fc_high(INTAN_config);

        fc_low_A(INTAN_config);
        fc_low_B(INTAN_config);



        power_up_AC(INTAN_config);
        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        amp_fast_settle_reset(INTAN_config);
        disable_U_flag(INTAN_config);

        enable_U_flag(INTAN_config);
        A_or_B_cutoff_frequency(INTAN_config);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);


        stim_step_DAC_configuration(INTAN_config);
        stim_PNBIAS_configuration(INTAN_config);
        charge_recovery_voltage_configuration(INTAN_config);
        charge_recovery_current_configuration(INTAN_config);
        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        all_stim_channels_off(INTAN_config);
        stimulation_on(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        stimulation_polarity(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        disconnect_channels_from_gnd(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        disable_charge_recovery_sw(INTAN_config);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        int i;
        for (i = NUM_CHANNELS-1; i>=0; i--){
                stim_current_channel_configuration(INTAN_config, i, INTAN_config->negative_current_trim[i],INTAN_config->negative_current_magnitude[i], INTAN_config->positive_current_trim[i], INTAN_config->positive_current_magnitude[i]);
        }
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);
        enable_M_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        clean_compliance_monitor(INTAN_config);
        send_SPI_commands(INTAN_config);
        disable_U_flag(INTAN_config);
        disable_M_flag(INTAN_config);
        send_SPI_commands(INTAN_config);

}



void INTAN_function_update(INTAN_config_struct* INTAN_config){

        read_command(INTAN_config, 255, '1');
        minimum_power_disipation(INTAN_config);
        stimulation_disable(INTAN_config);
        clear_command(INTAN_config);
        send_SPI_commands(INTAN_config);

        ADC_sampling_rate_config(INTAN_config);
        if (INTAN_config->C2_enabled){
                enable_C2(INTAN_config);
        }else{
                disable_C2(INTAN_config);
        }

        if (INTAN_config->abs_mode){
                enable_absolute_value(INTAN_config);
        }else{
                disable_absolute_value(INTAN_config);
        }

        if (INTAN_config->DSPen){
                enable_digital_signal_processing_HPF(INTAN_config);
                DSP_cutoff_frequency_configuration(INTAN_config);
        }else{
                disable_digital_signal_processing_HPF(INTAN_config);
        }


        disable_digital_output_1(INTAN_config);
        disable_digital_output_2(INTAN_config);
        power_OFF_output_1(INTAN_config);
        power_OFF_output_2(INTAN_config);

        impedance_check_control(INTAN_config);
        impedance_check_DAC(INTAN_config);

        
        fc_high(INTAN_config);

        fc_low_A(INTAN_config);
        fc_low_B(INTAN_config);

        power_up_AC(INTAN_config);
        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        amp_fast_settle_reset(INTAN_config);
        disable_U_flag(INTAN_config);

        enable_U_flag(INTAN_config);
        A_or_B_cutoff_frequency(INTAN_config);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        stim_step_DAC_configuration(INTAN_config);
        stim_PNBIAS_configuration(INTAN_config);
        charge_recovery_voltage_configuration(INTAN_config);
        charge_recovery_current_configuration(INTAN_config);
        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        all_stim_channels_off(INTAN_config);
        stimulation_on(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        stimulation_polarity(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        disconnect_channels_from_gnd(INTAN_config);
        disable_U_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        disable_charge_recovery_sw(INTAN_config);
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);

        enable_U_flag(INTAN_config);
        int i;
        for (i = NUM_CHANNELS-1; i>=0; i--){
                stim_current_channel_configuration(INTAN_config, i, INTAN_config->negative_current_trim[i],INTAN_config->negative_current_magnitude[i], INTAN_config->positive_current_trim[i], INTAN_config->positive_current_magnitude[i]);
        }
        disable_U_flag(INTAN_config);

        send_SPI_commands(INTAN_config);
        enable_M_flag(INTAN_config);
        enable_U_flag(INTAN_config);
        clean_compliance_monitor(INTAN_config);
        send_SPI_commands(INTAN_config);
        disable_U_flag(INTAN_config);
        disable_M_flag(INTAN_config);
        send_SPI_commands(INTAN_config);

}


#include "INTAN_config.h"

#include "values_macro.h"
#include "register_macro.h"


void call_configuration_functions(INTAN_config_struct* INTAN_config);
void call_sense_configuration_functions(INTAN_config_struct* INTAN_config, uint8_t channel);
void call_initialization_procedure_example(INTAN_config_struct* INTAN_config);
void call_initialization_procedure_example_test_INTAN_functions(INTAN_config_struct* INTAN_config);
void INTAN_function_update(INTAN_config_struct* INTAN_config);

#include <stdbool.h> 

#ifndef TIMER_CONFIG_H
#define TIMER_CONFIG_H
typedef struct{
    char operation_mode;
    bool interruptions;
    

    } TIMER_config_struct;


#endif

void setup_TIMER(TIMER_config_struct* TIMER_configuration);

#ifndef REGISTER_MACRO
#define REGISTER_MACRO

#define REGISTER_VALUE_TEST 0b01110000

// Stimulation enable
#define STIM_ENABLE_A 0x20 //32
#define STIM_ENABLE_B 0x21 //33

// Stimulation step size
#define STIM_STEP_SIZE 0x22 //34

// Stimulation Bias Voltages
#define STIM_BIAS_VOLTAGE 0x23 //35

// Current-Limited Charge Recovery Target Voltage
#define CURRENT_LIMITED_CHARGE_RECOVERY 0x24 //36

// Charge Recovery Current Limit 
#define CHARGE_RECOVERY_CURRENT_LIMIT 0x25 //37

// Individual DC Amplifier Power 
#define DC_AMPLIFIER_POWER 0x26 //38

//  Compliance Monitor (READ ONLY REGISTER WITH CLEAR) 
#define CIMPLIANCE_MONITOR 0x28 //40

// Stimulator On (TRIGGERED REGISTER) 
#define STIMULATOR_ON 0x2A //42

//Stimulator Polarity (TRIGGERED REGISTER) 
#define STIMULATOR_ON 0x2C //44

//Charge Recovery Switch (TRIGGERED REGISTER)
#define STIMULATOR_ON 0x2E //46

// Current-Limited Charge Recovery Enable (TRIGGERED REGISTER)
#define STIMULATOR_ON 0x30 //48

// Fault Current Detector (READ ONLY REGISTER) 
#define STIMULATOR_ON 0x32 //50

//Registers 64-79: Negative Stimulation Current Magnitude (TRIGGERED REGISTERS)

//Registers 96-111: Positive Stimulation Current Magnitude (TRIGGERED REGISTERS) 
#endif


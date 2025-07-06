#ifndef REGISTER_MACRO
#define REGISTER_MACRO


#define WRITE_ACTION                    0b10000000
#define WRITE_ACTION_M                  0b10010000
#define WRITE_ACTION_U                  0b10100000

#define READ_ACTION                     0b10000000
#define READ_ACTION_M                   0b11010000
#define READ_ACTION_U                   0b11100000

#define REGISTER_VALUE_TEST             0b01110000

// Stimulation enable
#define STIM_ENABLE_A                   0x20 //32
#define STIM_ENABLE_B                   0x21 //33

// Stimulation step size
#define STIM_STEP_SIZE                  0x22 //34

// Stimulation Bias Voltages
#define STIM_BIAS_VOLTAGE               0x23 //35

// Current-Limited Charge Recovery Target Voltage
#define CURRENT_LIMITED_CHARGE_RECOVERY 0x24 //36

// Charge Recovery Current Limit 
#define CHARGE_RECOVERY_CURRENT_LIMIT   0x25 //37

// Individual DC Amplifier Power 
#define DC_AMPLIFIER_POWER              0x26 //38

//  Compliance Monitor (READ ONLY REGISTER WITH CLEAR) 
#define COMPLIANCE_MONITOR              0x28 //40

// Stimulator On (TRIGGERED REGISTER) 
#define STIM_ON                         0x2A //42

//Stimulator Polarity (TRIGGERED REGISTER) 
#define STIM_POLARITY                   0x2C //44

//Charge Recovery Switch (TRIGGERED REGISTER)
#define CHRG_RECOV_SWITCH               0x2E //46

// Current-Limited Charge Recovery Enable (TRIGGERED REGISTER)
#define CURR_LIM_CHRG_RECOV_EN          0x30 //48

// Fault Current Detector (READ ONLY REGISTER) 
#define FAULT_CURR_DET                  0x32 //50

//Registers 64-79: Negative Stimulation Current Magnitude (TRIGGERED REGISTERS)
#define CH0_NEG_CURR_MAG                0x40 //64
#define CH1_NEG_CURR_MAG                0x41 //65
#define CH2_NEG_CURR_MAG                0x42 //66
#define CH3_NEG_CURR_MAG                0x43 //67
#define CH4_NEG_CURR_MAG                0x44 //68
#define CH5_NEG_CURR_MAG                0x45 //69
#define CH6_NEG_CURR_MAG                0x46 //70
#define CH7_NEG_CURR_MAG                0x47 //71
#define CH8_NEG_CURR_MAG                0x48 //72
#define CH9_NEG_CURR_MAG                0x49 //73
#define CH10_NEG_CURR_MAG               0x4A //74
#define CH11_NEG_CURR_MAG               0x4B //75
#define CH12_NEG_CURR_MAG               0x4C //76
#define CH13_NEG_CURR_MAG               0x4D //77
#define CH14_NEG_CURR_MAG               0x4E //78
#define CH15_NEG_CURR_MAG               0x4F //79

//Registers 96-111: Positive Stimulation Current Magnitude (TRIGGERED REGISTERS) 
#define CH0_POS_CURR_MAG                0x60 //96
#define CH1_POS_CURR_MAG                0x61 //97
#define CH2_POS_CURR_MAG                0x62 //98
#define CH3_POS_CURR_MAG                0x63 //99
#define CH4_POS_CURR_MAG                0x64 //101
#define CH5_POS_CURR_MAG                0x65 //102
#define CH6_POS_CURR_MAG                0x66 //103
#define CH7_POS_CURR_MAG                0x67 //104
#define CH8_POS_CURR_MAG                0x68 //105
#define CH9_POS_CURR_MAG                0x69 //106
#define CH10_POS_CURR_MAG               0x6A //107
#define CH11_POS_CURR_MAG               0x6B //108
#define CH12_POS_CURR_MAG               0x6C //109
#define CH13_POS_CURR_MAG               0x6D //110
#define CH14_POS_CURR_MAG               0x6E //111
#define CH15_POS_CURR_MAG               0x6F //112
#endif


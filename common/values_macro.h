#ifndef VALUES_MACRO
#define VALUES_MACRO

#define VALUES_VALUE_TEST_H 0b00100000
#define VALUES_VALUE_TEST_L 0b00000011

#define ZEROS_8 0x00
#define ONES_8  0xFF

// Stimulation enable
#define STIM_ENABLE_A_VALUE_H 0xAA
#define STIM_ENABLE_A_VALUE_L 0xAA

#define STIM_DISABLE_VALUE_H 0x00
#define STIM_DISABLE_VALUE_L 0x00

#define STIM_ENABLE_B_VALUE_H 0x00
#define STIM_ENABLE_B_VALUE_L 0xFF

// Step selection
// 7 bits for SEL 1
#define SEL1_10nA  64 
#define SEL1_20nA  40
#define SEL1_50nA  64
#define SEL1_100nA 30
#define SEL1_200nA 25
#define SEL1_500nA 101
#define SEL1_1uA   98
#define SEL1_2uA   94
#define SEL1_5uA   38
#define SEL1_10uA  15

// 6 bits for SEL 1
#define SEL2_10nA  19
#define SEL2_20nA  40
#define SEL2_50nA  40
#define SEL2_100nA 20
#define SEL2_200nA 10
#define SEL2_500nA 3
#define SEL2_1uA   1
#define SEL2_2uA   0
#define SEL2_5uA   0
#define SEL2_10uA  0

// 2 bits for SEL 1
#define SEL3_10nA  3
#define SEL3_20nA  1
#define SEL3_50nA  0
#define SEL3_100nA 0
#define SEL3_200nA 0
#define SEL3_500nA 0
#define SEL3_1uA   0
#define SEL3_2uA   0
#define SEL3_5uA   0
#define SEL3_10uA  0




// STIM PBIAS and NBIAS - Bias Voltage
//PBIAS
#define PBIAS_10nA   6
#define PBIAS_20nA   7
#define PBIAS_50nA   7
#define PBIAS_100nA  7
#define PBIAS_200nA  8
#define PBIAS_500nA  9
#define PBIAS_1uA    10
#define PBIAS_2uA    11
#define PBIAS_5uA    4
#define PBIAS_10uA   15

//NBIAS
#define NBIAS_10nA   6
#define NBIAS_20nA   7
#define NBIAS_50nA   7
#define NBIAS_100nA  7
#define NBIAS_200nA  8
#define NBIAS_500nA  9
#define NBIAS_1uA    10
#define NBIAS_2uA    11
#define NBIAS_5uA    4
#define NBIAS_10uA   15


//Current limited charge recovery
#define CL_SEL1_1nA   0
#define CL_SEL1_2nA   0
#define CL_SEL1_5nA   0
#define CL_SEL1_10nA  50
#define CL_SEL1_20nA  78
#define CL_SEL1_50nA  22
#define CL_SEL1_100nA 56
#define CL_SEL1_200nA 71
#define CL_SEL1_500nA 26
#define CL_SEL1_1uA   9

#define CL_SEL2_1nA   30
#define CL_SEL2_2nA   15
#define CL_SEL2_5nA   31
#define CL_SEL2_10nA  15
#define CL_SEL2_20nA  7
#define CL_SEL2_50nA  3
#define CL_SEL2_100nA 1
#define CL_SEL2_200nA 0
#define CL_SEL2_500nA 0
#define CL_SEL2_1uA   0

#define CL_SEL3_1nA   2
#define CL_SEL3_2nA   1
#define CL_SEL3_5nA   0
#define CL_SEL3_10nA  0
#define CL_SEL3_20nA  0
#define CL_SEL3_50nA  0
#define CL_SEL3_100nA 0
#define CL_SEL3_200nA 0
#define CL_SEL3_500nA 0
#define CL_SEL3_1uA   0

//Stimulaiton ON or OFF and CHRG RECOV ON or OFF in each channel
#define ALL_CH_OFF      ZEROS_8

#define CH_0_ON_L       0b00000001
#define CH_1_ON_L       0b00000010
#define CH_2_ON_L       0b00000100
#define CH_3_ON_L       0b00001000
#define CH_4_ON_L       0b00010000
#define CH_5_ON_L       0b00100000
#define CH_6_ON_L       0b01000000
#define CH_7_ON_L       0b10000000
#define CH_8_ON_H       0b00000001
#define CH_9_ON_H       0b00000010
#define CH_10_ON_H      0b00000100
#define CH_11_ON_H      0b00001000
#define CH_12_ON_H      0b00010000
#define CH_13_ON_H      0b00100000
#define CH_14_ON_H      0b01000000
#define CH_15_ON_H      0b10000000



#endif


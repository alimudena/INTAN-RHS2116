#ifndef VALUES_MACRO
#define VALUES_MACRO

#define VALUES_VALUE_TEST_H 0b00100000
#define VALUES_VALUE_TEST_L 0b00000011

#define ZEROS_8 0x00
#define ONES_8  0xFF

// Clear function returns
#define ZEROS_32 0x00000000
#define RETURN_CLEAR_2C 0b10000000000000000000000000000000

// Chip ID
#define CHIP_ID 0x00000020


// Cutoff Frequency high
#define RH1_SEL1_20K           0x08  //8
#define RH1_SEL1_15K           0x0B  //11
#define RH1_SEL1_10K           0x11  //17
#define RH1_SEL1_7_5K          0x16  //22
#define RH1_SEL1_5K            0x21  //33
#define RH1_SEL1_3K            0x03  //3
#define RH1_SEL1_2_5K          0x0D  //13
#define RH1_SEL1_2K            0x1B  //27
#define RH1_SEL1_1_5K          0x01  //1
#define RH1_SEL1_1K            0x2E  //46
#define RH1_SEL1_750H          0x29  //41
#define RH1_SEL1_500H          0x1E  //30
#define RH1_SEL1_300H          0x06  //6
#define RH1_SEL1_250H          0x2A  //42
#define RH1_SEL1_200H          0x18  //24
#define RH1_SEL1_150H          0x2C  //44
#define RH1_SEL1_100H          0x26  //38

#define RH1_SEL2_20K           0x00  //0
#define RH1_SEL2_15K           0x00  //0
#define RH1_SEL2_10K           0x00  //0
#define RH1_SEL2_7_5K          0x00  //0
#define RH1_SEL2_5K            0x00  //0
#define RH1_SEL2_3K            0x01  //1
#define RH1_SEL2_2_5K          0x01  //1
#define RH1_SEL2_2K            0x01  //1
#define RH1_SEL2_1_5K          0x02  //2
#define RH1_SEL2_1K            0x02  //2
#define RH1_SEL2_750H          0x03  //3
#define RH1_SEL2_500H          0x05  //5
#define RH1_SEL2_300H          0x09  //9
#define RH1_SEL2_250H          0x0A  //10
#define RH1_SEL2_200H          0x0D  //13
#define RH1_SEL2_150H          0x11  //17
#define RH1_SEL2_100H          0x1A  //26

#define RH2_SEL1_20K           0x04  //4
#define RH2_SEL1_15K           0x08  //8
#define RH2_SEL1_10K           0x10  //16
#define RH2_SEL1_7_5K          0x17  //23
#define RH2_SEL1_5K            0x25  //37
#define RH2_SEL1_3K            0x0D  //13
#define RH2_SEL1_2_5K          0x19  //25
#define RH2_SEL1_2K            0x2C  //44
#define RH2_SEL1_1_5K          0x17  //23
#define RH2_SEL1_1K            0x1E  //30
#define RH2_SEL1_750H          0x24  //36
#define RH2_SEL1_500H          0x2B  //43
#define RH2_SEL1_300H          0x02  //2
#define RH2_SEL1_250H          0x05  //5
#define RH2_SEL1_200H          0x07  //7
#define RH2_SEL1_150H          0x08  //8
#define RH2_SEL1_100H          0x05  //5

#define RH2_SEL2_20K           0x00  //0
#define RH2_SEL2_15K           0x00  //0
#define RH2_SEL2_10K           0x00  //0
#define RH2_SEL2_7_5K          0x00  //0
#define RH2_SEL2_5K            0x00  //0
#define RH2_SEL2_3K            0x01  //1
#define RH2_SEL2_2_5K          0x01  //1
#define RH2_SEL2_2K            0x01  //1
#define RH2_SEL2_1_5K          0x02  //2
#define RH2_SEL2_1K            0x03  //3
#define RH2_SEL2_750H          0x04  //4
#define RH2_SEL2_500H          0x06  //6
#define RH2_SEL2_300H          0x0B  //11
#define RH2_SEL2_250H          0x0D  //13
#define RH2_SEL2_200H          0x10  //16
#define RH2_SEL2_150H          0x15  //21
#define RH2_SEL2_100H          0x1F  //31

// Cutoff Frequency low
#define      RL_SEL1_1k           0x0A
#define      RL_SEL1_500H         0x0D
#define      RL_SEL1_300H         0x0F
#define      RL_SEL1_250H         0x11
#define      RL_SEL1_200H         0x12
#define      RL_SEL1_150H         0x15
#define      RL_SEL1_100H         0x19
#define      RL_SEL1_75H          0x1C
#define      RL_SEL1_50H          0x22
#define      RL_SEL1_30H          0x2C
#define      RL_SEL1_25H          0x30
#define      RL_SEL1_20H          0x36
#define      RL_SEL1_15H          0x3E
#define      RL_SEL1_10H          0x05
#define      RL_SEL1_7_5H         0x12
#define      RL_SEL1_5H           0x28
#define      RL_SEL1_3H           0x14
#define      RL_SEL1_2_5H         0x2A
#define      RL_SEL1_2H           0x08
#define      RL_SEL1_1_5H         0x09
#define      RL_SEL1_1H           0x2C
#define      RL_SEL1_0_75H        0x31
#define      RL_SEL1_0_5H         0x23
#define      RL_SEL1_0_3H         0x01
#define      RL_SEL1_0_25H        0x38
#define      RL_SEL1_0_1H         0x10

#define      RL_SEL2_1k           0x00
#define      RL_SEL2_500H         0x00
#define      RL_SEL2_300H         0x00
#define      RL_SEL2_250H         0x00
#define      RL_SEL2_200H         0x00
#define      RL_SEL2_150H         0x00
#define      RL_SEL2_100H         0x00
#define      RL_SEL2_75H          0x00
#define      RL_SEL2_50H          0x00
#define      RL_SEL2_30H          0x00
#define      RL_SEL2_25H          0x00
#define      RL_SEL2_20H          0x00
#define      RL_SEL2_15H          0x00
#define      RL_SEL2_10H          0x01
#define      RL_SEL2_7_5H         0x01
#define      RL_SEL2_5H           0x01
#define      RL_SEL2_3H           0x02
#define      RL_SEL2_2_5H         0x02
#define      RL_SEL2_2H           0x03
#define      RL_SEL2_1_5H         0x04
#define      RL_SEL2_1H           0x06
#define      RL_SEL2_0_75H        0x09
#define      RL_SEL2_0_5H         0x11
#define      RL_SEL2_0_3H         0x28
#define      RL_SEL2_0_25H        0x36
#define      RL_SEL2_0_1H         0x3C

#define      RL_SEL3_1k         0x00
#define      RL_SEL3_500H         0x00
#define      RL_SEL3_300H         0x00
#define      RL_SEL3_250H         0x00
#define      RL_SEL3_200H         0x00
#define      RL_SEL3_150H         0x00
#define      RL_SEL3_100H         0x00
#define      RL_SEL3_75H         0x00
#define      RL_SEL3_50H         0x00
#define      RL_SEL3_30H         0x00
#define      RL_SEL3_25H         0x00
#define      RL_SEL3_20H         0x00
#define      RL_SEL3_15H         0x00
#define      RL_SEL3_10H         0x00
#define      RL_SEL3_7_5H         0x00
#define      RL_SEL3_5H         0x00
#define      RL_SEL3_3H         0x00
#define      RL_SEL3_2_5H         0x00
#define      RL_SEL3_2H         0x00
#define      RL_SEL3_1_5H         0x00
#define      RL_SEL3_1H         0x00
#define      RL_SEL3_0_75H         0x00
#define      RL_SEL3_0_5H         0x00
#define      RL_SEL3_0_3H         0x00
#define      RL_SEL3_0_25H         0x00
#define      RL_SEL3_0_1H         0x01


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


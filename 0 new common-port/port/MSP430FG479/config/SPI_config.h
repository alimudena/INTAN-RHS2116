#include <stdbool.h> 

#ifndef SPI_CONFIG_H
#define SPI_CONFIG_H
typedef struct{
    /*Selection if the controller works as Master (M) or Slave (S)*/
    char Master_Slave;

    // clock polarity inactive low - 'L'
    // clock polarity inactive high - 'H'
    char inactive_state;
    
    //data cHanged on the first UCLK edge and captured on the following edge
    //data cAptured on the first UCLK edge and changed on the following edge
    char data_on_clock_edge;

    // 7 or 8 bits of information
    int SPI_length;

    //Most significant bit sent first
    //Least significant bit sent first
    char first_Byte_sent;

    /*clk_ref:
    U --> UCLK
    A --> ACLK
    S --> SMCLK        
    */
    char clk_ref_SPI;
    
    
    int clk_div;

    // Enable USCI_A0 RX interrupt
    // Enable USCI_A0 TX interrupt
    bool enable_USCI_interr_rx; 
    bool enable_USCI_interr_tx;
} SPI_config_struct;


typedef struct{
    /*Selection if the controller works as Master (M) or Slave (S)*/
    char Master_Slave;

    // clock polarity inactive low - 'L'
    // clock polarity inactive high - 'H'
    char inactive_state;
    
    //data cHanged on the first UCLK edge and captured on the following edge
    //data cAptured on the first UCLK edge and changed on the following edge
    char data_on_clock_edge;

    // 7 or 8 bits of information
    int SPI_length;

    //Most significant bit sent first
    //Least significant bit sent first
    char first_Byte_sent;

    /*clk_ref:
    U --> UCLK
    A --> ACLK
    S --> SMCLK        
    */
    char clk_ref_SPI;
    
    
    int clk_div;

    // Enable USCI_A0 RX interrupt
    // Enable USCI_A0 TX interrupt
    bool enable_USCI_interr_rx; 
    bool enable_USCI_interr_tx;
} SPI_B_config_struct;
#endif

void SPI_setup(SPI_config_struct* SPI_config);
void SPI_B_setup(SPI_B_config_struct* SPI_config);


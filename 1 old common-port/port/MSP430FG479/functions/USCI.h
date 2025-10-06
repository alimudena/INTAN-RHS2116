#include <stdint.h>
#include <msp430.h>
#include <stdbool.h> 

//How to use UART comms steps
//The configuration steps for the USCI initialization/re-configuration process is:
//First set UCSWRST --> USCI_reset()

//Initialize USCI registers with UCSWRST = 1 --> Rest of the functions

//Configure ports --> TODO

//Clear UCSWRST --> USCI_init()
//Enable interrupts (optional)

//*****************************************************************************
/*USCI INITIALIZATION AND RESET FUNCTIONS*/
//*****************************************************************************

void USCI_reset();
void USCI_setup();
void USCI_init();

void init_UART_GPIO();

void USCI_clk_ref(char clk_ref);

void USCI_interrupt_enable(bool enable_USCI_interr_rx, bool enable_USCI_interr_tx);


//*****************************************************************************
/*UART RELATED FUNCTIONS*/
//*****************************************************************************
// character format 
    //parity enable:
        //True --> parity ON
        //False --> parity OFF            
    //if parity enable:
        //parity_type:
            //E --> even (par)
            //O --> odd (impar)
    //num_data_bit: quantity of data bits available (character length)
        //7 or 8
    //num_stop_bit: stop bit select, one at least
        //1 or 2
    //first_byte_sent: To choose between MSB or LSB 
        //M: MSB first
        //L: LSB first


void character_format_sel(bool parity_enable, char parity_type, int num_data_bit, int num_stop_bit, char first_Byte_sent);

// USCI mode
void USCI_mode_sel(char USCI_mode);

void data_to_transmit(uint8_t data);

//void UART_baudrate_generation(int ref_frec, int baudrate_wanted);
void UART_baudrate_generation(int32_t BRCLK_freq, int32_t baudrate);



// IrDA encoding(decoding) enable
void IrDA_enable(bool IrDA_enabled);
// Encoding
void IrDA_pulse_config(char IrDA_clk_ref, int IrDA_Transmit_Pulse_Length);

// Decoding
void IrDA_detect_value(int H_L_detect);
void IrDA_decoding_filter(bool IrDA_dec_filter_enabled, int IrDA_Receive_Filter_Length);


//*****************************************************************************
/*SPI RELATED FUNCTIONS*/
//*****************************************************************************

void SPI_char_format(int SPI_length, char first_Byte_sent);

void USCI_SPI_pin_setup();

void SPI_mode_config(char Master_Slave);

void SPI_clk_division(int clk_div);

void SPI_clk_polarity_phase(char inactive_state, char data_on_clock_edge);

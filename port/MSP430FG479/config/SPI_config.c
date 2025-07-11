#include "SPI_config.h"
#include "../functions/USCI.h"
#include "../IKKI/IKKI_MAC.h"

void SPI_setup(SPI_config_struct* SPI_config) {

  USCI_SPI_pin_setup();


  SPI_mode_config(SPI_config->Master_Slave);


  SPI_clk_polarity_phase(SPI_config->inactive_state,
                         SPI_config->data_on_clock_edge);


  SPI_char_format(SPI_config->SPI_length,
                  SPI_config->first_Byte_sent); // 8-bit and MSB SPI


  USCI_clk_ref(SPI_config->clk_ref_SPI); // CLK reference


  SPI_clk_division(SPI_config->clk_div);

  USCI_init(); // **Initialize USCI state machine**


  USCI_interrupt_enable(
      SPI_config->enable_USCI_interr_rx,
      SPI_config->enable_USCI_interr_tx); // Enable USCI_A0 RX interrupt
}

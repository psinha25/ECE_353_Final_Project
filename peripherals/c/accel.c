#include "accel.h"

extern void spiTx(uint32_t base, uint8_t *tx_data, int size, uint8_t *rx_data);
extern bool spiVerifyBaseAddr(uint32_t base);


//*****************************************************************************
// Manually sets the SPI chip select line low
//*****************************************************************************
static __INLINE void  accel_CSN_low(void)
{
  // ADD CODE
}

//*****************************************************************************
// Manually sets the SPI chip select line high
//*****************************************************************************
static __INLINE void  accel_CSN_high(void)
{
  // ADD CODE
}



//*****************************************************************************
// Read a register from the LSM6DS3H
//*****************************************************************************
static __INLINE uint8_t accel_reg_read(uint8_t reg)
{
  // ADD CODE

  return 0; // Modify to return the register value
}

//*****************************************************************************
// Write a register from the LSM6DS3H
//*****************************************************************************
static __INLINE void accel_reg_write(uint8_t reg, uint8_t data)
{
  // ADD CODE
}


//*****************************************************************************
// Read the X acceleration reading.
//*****************************************************************************
int16_t accel_read_x(void)
{
  // ADD CODE
  return 0; // Modify to return the x value
  
} 

//*****************************************************************************
// Read the Y acceleration reading.
//*****************************************************************************
int16_t accel_read_y(void)
{
  // ADD CODE
  return 0; // Modify to return the Y value
  
}

//*****************************************************************************
// Read the Z acceleration reading.
//*****************************************************************************
int16_t accel_read_z(void)
{
  // ADD CODE
  return 0; // Modify to return the Z value
  
}

//*****************************************************************************
// Used to initialize the GPIO pins used to connect to the LSM6DS3H.
//
// Configuration Info
//		Fill out relevant information in accel.h.  accel.h defines 
//		how various peripherals are physically connected to the board.
//  
//*****************************************************************************
void accel_initialize(void)
{  
  int i = 0;
	spi_select_init();
	spi_select(MODULE_1);
	
  gpio_enable_port(ACCEL_GPIO_BASE);
  
  // Configure SPI CLK
  gpio_config_digital_enable(ACCEL_GPIO_BASE, ACCEL_CLK_PIN);
  gpio_config_alternate_function(ACCEL_GPIO_BASE, ACCEL_CLK_PIN);
  gpio_config_port_control(ACCEL_GPIO_BASE, ACCEL_SPI_CLK_PCTL_M, ACCEL_CLK_PIN_PCTL);
  
  // Configure SPI MISO
  gpio_config_digital_enable(ACCEL_GPIO_BASE, ACCEL_MISO_PIN);
  gpio_config_alternate_function(ACCEL_GPIO_BASE, ACCEL_MISO_PIN);
  gpio_config_port_control(ACCEL_GPIO_BASE, ACCEL_SPI_MISO_PCTL_M, ACCEL_MISO_PIN_PCTL);
  
  // Configure SPI MOSI
  gpio_config_digital_enable(ACCEL_GPIO_BASE, ACCEL_MOSI_PIN);
  gpio_config_alternate_function(ACCEL_GPIO_BASE, ACCEL_MOSI_PIN);
  gpio_config_port_control(ACCEL_GPIO_BASE, ACCEL_SPI_MOSI_PCTL_M, ACCEL_MOSI_PIN_PCTL);
  
  // Configure CS to be a normal GPIO pin that is controlled 
  // explicitly by software
  gpio_enable_port(ACCEL_CS_BASE);
  gpio_config_digital_enable(ACCEL_CS_BASE,ACCEL_CS_PIN);
  gpio_config_enable_output(ACCEL_CS_BASE,ACCEL_CS_PIN);

  initialize_spi( ACCEL_SPI_BASE, ACCEL_SPI_MODE, 10);
	
	while( accel_reg_read(ACCEL_WHO_AM_I_R) != 0x69)
	{
			for(i=0; i < 1000; i++)
			{};
	}

	accel_reg_write(ACCEL_INT1_CTRL_R, 0x00);  // For now, disable interrupts
	
	accel_reg_write(ACCEL_CTRL1_XL_R, ACCEL_CTRL1_XL_ODR_208HZ | ACCEL_CTRL1_XL_2G | ACCEL_CTRL1_XL_ANTI_ALIAS_50HZ);
	accel_reg_write(ACCEL_CTRL2_G_R, ACCEL_CTRL2_G_ODR_416HZ | ACCEL_CTRL2_G_FS_245_DPS | ACCEL_CTRL2_G_FS_125);
	accel_reg_write(ACCEL_CTRL5_C_R, ACCEL_CTRL5_SLEEP_G | ACCEL_CTRL5_INT2_ON_INT1);
	
}

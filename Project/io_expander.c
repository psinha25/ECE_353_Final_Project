#include "io_expander.h"

//*****************************************************************************
// Writes a single byte of data out to the MCP23017
//
// Paramters
//    i2c_base:   a valid base address of an I2C peripheral
//
//    address:    8-bit address of the byte being written
//
//    data:       Data written to the MCP23017.
//
// Returns
// I2C_OK if the byte was written to the MCP23017.
//*****************************************************************************
static i2c_status_t io_expander_byte_write (
	uint32_t i2c_base, 
	uint8_t address, 
	uint8_t data
)
{
	i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while (I2CMasterBusy(i2c_base)) {};
	
  // Set the I2C address 
	status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_WRITE);
  if (status != I2C_OK) return status;
	
  // Send the address
	status = i2cSendByte(i2c_base, address, I2C_MCS_START | I2C_MCS_RUN);
	if (status != I2C_OK) return status;
	
	 // Send the byte of data to write
	status = i2cSendByte(i2c_base, data, I2C_MCS_RUN | I2C_MCS_STOP);
	
  return status;
	
}	

//*****************************************************************************
// Configure pins associated with directional push buttons to generate 
// interrupts when pushed
//*****************************************************************************
static void configure_button_interrupts(void)
{
	uint8_t unnecessary_data; 
	//get_button_data(&unnecessary_data); 
	io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_GPINTENB_R, 0x0F); // enable interrupts 
	io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_DEFVALB_R, 0x0F);  // set default to 1 because interrupt is active low
	io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_INTCONB_R, 0x00);  // 0xFF
	get_button_data(&unnecessary_data); 
//	printf("\n\r%i", unnecessary_data); 
//	get_button_data_if(&unnecessary_data); 
//	printf("\n\r%i", unnecessary_data); 
	
	gpio_enable_port(IO_EXPANDER_IRQ_GPIO_BASE);
	gpio_config_digital_enable(IO_EXPANDER_IRQ_GPIO_BASE, IO_EXPANDER_IRQ_PIN_NUM); 
	gpio_config_enable_input(IO_EXPANDER_IRQ_GPIO_BASE, IO_EXPANDER_IRQ_PIN_NUM);
	// gpio_config_enable_pullup(IO_EXPANDER_IRQ_GPIO_BASE, IO_EXPANDER_IRQ_PIN_NUM); 
	gpio_config_falling_edge_irq(IO_EXPANDER_IRQ_GPIO_BASE, IO_EXPANDER_IRQ_PIN_NUM);
	//get_button_data(&unnecessary_data); 

}

//*****************************************************************************
// Configure IO Expander LEDs and Write to them
//*****************************************************************************
void write_leds(uint8_t data)
{
	io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_IODIRA_R, 0x00); 
	io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_GPIOA_R, data);
}

//*****************************************************************************
// Configure the MCP23017 to take input from the four directional buttons.
//*****************************************************************************
i2c_status_t configure_buttons(void)
{
	i2c_status_t status;
	
	// Configure GPIOB port as input
	status = io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_IODIRB_R, 0xFF);
	if (status != I2C_OK) return status;
	
	// Enable pull-up resistors
	status = io_expander_byte_write(IO_EXPANDER_I2C_BASE, MCP23017_GPPUB_R, 0x0F);
	
	configure_button_interrupts(); 
	
	return status;
}

//*****************************************************************************
// Get input from the four directional buttons.
//*****************************************************************************
i2c_status_t get_button_data(uint8_t *data)
{
  i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while (I2CMasterBusy(IO_EXPANDER_I2C_BASE)) {};

  // Set the I2C slave address to be the MCP23017 and in Write Mode
	status = i2cSetSlaveAddr(IO_EXPANDER_I2C_BASE, MCP23017_DEV_ID, I2C_WRITE);
  if (status != I2C_OK) return status;
	
	// Send the address of the GPIOB registers
	// status = i2cSendByte(IO_EXPANDER_I2C_BASE, MCP23017_INTFB_R, I2C_MCS_START | I2C_MCS_RUN);
	status = i2cSendByte(IO_EXPANDER_I2C_BASE, MCP23017_GPIOB_R, I2C_MCS_START | I2C_MCS_RUN);
  if (status != I2C_OK) return status;
	
	// Set the I2C slave address to be the MCP23017 and in Read Mode
	status = i2cSetSlaveAddr(IO_EXPANDER_I2C_BASE, MCP23017_DEV_ID, I2C_READ);
  if (status != I2C_OK) return status;
	
  // Get the data returned by the MCP23017 GPIOB port (button data)
	status = i2cGetByte(IO_EXPANDER_I2C_BASE, data, I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP);

  return status;
}


i2c_status_t get_button_data_if(uint8_t *data)
{
	i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while (I2CMasterBusy(IO_EXPANDER_I2C_BASE)) {};

  // Set the I2C slave address to be the MCP23017 and in Write Mode
	status = i2cSetSlaveAddr(IO_EXPANDER_I2C_BASE, MCP23017_DEV_ID, I2C_WRITE);
  if (status != I2C_OK) return status;
	
	// Send the address of the GPIOB registers
	status = i2cSendByte(IO_EXPANDER_I2C_BASE, MCP23017_INTFB_R, I2C_MCS_START | I2C_MCS_RUN);
	// status = i2cSendByte(IO_EXPANDER_I2C_BASE, MCP23017_GPIOB_R, I2C_MCS_START | I2C_MCS_RUN);
  if (status != I2C_OK) return status;
	
	// Set the I2C slave address to be the MCP23017 and in Read Mode
	status = i2cSetSlaveAddr(IO_EXPANDER_I2C_BASE, MCP23017_DEV_ID, I2C_READ);
  if (status != I2C_OK) return status;
	
  // Get the data returned by the MCP23017 GPIOB port (button data)
	status = i2cGetByte(IO_EXPANDER_I2C_BASE, data, I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP);

  return status;
	
	
}

//*****************************************************************************
// Initialize the I2C peripheral
//*****************************************************************************
bool io_expander_init(void)
{
	
	// Configure I2C GPIO Pins
	if(gpio_enable_port(IO_EXPANDER_GPIO_BASE) == false)
  {
    return false;
  }
	
	// Configure SCL 
  if(gpio_config_digital_enable(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_alternate_function(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_port_control(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PCTL_M, IO_EXPANDER_I2C_SCL_PIN_PCTL)== false)
  {
    return false;
  }
	
	// Configure SDA 
  if(gpio_config_digital_enable(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_open_drain(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_alternate_function(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_port_control(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PCTL_M, IO_EXPANDER_I2C_SDA_PIN_PCTL)== false)
  {
    return false;
  }
	
	//  Initialize the I2C peripheral
  if( initializeI2CMaster(IO_EXPANDER_I2C_BASE)!= I2C_OK)
  {
    return false;
  }
	
}
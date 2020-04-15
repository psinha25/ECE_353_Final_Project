// Copyright (c) 2014-16, Joe Krachey
// All rights reserved.
//
// Redistribution and use in binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "ps2-new.h"


/*******************************************************************************
* Function Name: initialize_adc_gpio_pins
********************************************************************************
* Used to initialize the PS2 joystick GPIO pins for both the analog inputs
*
* Configuration Info
*		Fill out relevant information in ps2.h.  ps2.h defines 
*		how various peripherals are physically connected to the board.
*******************************************************************************/
static void initialize_adc_gpio_pins(void)
{
  gpio_enable_port(PS2_GPIO_ANALOG_BASE);
  gpio_config_enable_input(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
  gpio_config_analog_enable(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
  gpio_config_alternate_function(PS2_GPIO_ANALOG_BASE, PS2_X_DIR_PIN | PS2_Y_DIR_PIN);
}


/*******************************************************************************
* Function Name: ps2_configure_digital_comparitors
********************************************************************************
* Summary:
* Configures SSMUX2 to convert the raw ADC values.
*
* Return:
*  void
*
*******************************************************************************/
static void ps2_configure_adc(void)
{
 // Configure SSMUX2 to read the raw ADC values
		
  ADC0->SSMUX2 = PS2_X_DIR_CH << 4 | PS2_Y_DIR_CH;  // Set Channels
    
  // Enable the interrupts for SS2
  ADC0->IM |= ADC_IM_MASK2;
  
  // Set the end of the sequence and when to generate interrupts
  ADC0->SSCTL2 = ADC_SSCTL2_IE1 | ADC_SSCTL2_END1;
  
  NVIC_EnableIRQ(ADC0SS2_IRQn);
  NVIC_SetPriority(ADC0SS2_IRQn ,3);
  
  ADC0->ACTSS |= ADC_ACTSS_ASEN2;  // Enable SS2
  
}
//*****************************************************************************
// Initializes the ADC0 to use SS2 to convert both the X and Y pots for the
// PS2 joystick.  The ADC is triggered by TIMER4 interrupts.
//*****************************************************************************
bool ps2_initialize_adc(void)
{

 // Turn on the ADC Clock
  SYSCTL->RCGCADC |= SYSCTL_RCGCADC_R0;
  
  // Wait for ADCx to become ready
  while( (SYSCTL_PRADC_R0 & SYSCTL->PRADC) != SYSCTL_PRADC_R0){}
  
  // disable all the sample sequencers
  ADC0->ACTSS &= ~(ADC_ACTSS_ASEN0 | ADC_ACTSS_ASEN1| ADC_ACTSS_ASEN2| ADC_ACTSS_ASEN3);

	ADC0->ISC = 0xFFFFFFFF;

  // Sequencer 3 is the lowest priority
  ADC0->SSPRI = ADC_SSPRI_SS3_4TH | ADC_SSPRI_SS2_3RD | ADC_SSPRI_SS1_2ND | ADC_SSPRI_SS0_1ST;

	// Set all the sample sequencers to be triggered by software.
  ADC0->EMUX = 0 ;
  
    // Clear Averaging Bits
  ADC0->SAC &= ~ADC_SAC_AVG_M  ;
  
  // Average 64 samples
  ADC0->SAC |= ADC_SAC_AVG_8X;

#if DCOMP_ENABLE    
  ps2_configure_digital_comparitors();
#endif

  ps2_configure_adc();
  
  return true;
}


/*******************************************************************************
* Function Name: ps2_initialize
********************************************************************************
* Initializes the GPIO pins connected to the PS2 Joystick.  It also configures
* ADC0 to use Sample Sequencer #3 to convert a programmable channel number.
*******************************************************************************/
void ps2_initialize(void)
{
	initialize_adc_gpio_pins();
	ps2_initialize_adc();
}

/*******************************************************************************
* Function Name: ps2_get_x
********************************************************************************
*Returns the most current reading of the X direction  Only the lower 12-bits
* contain data.
********************************************************************************/
uint16_t ps2_get_x(void)
{
  uint16_t adc_val;
  
  return adc_val;
}

/*******************************************************************************
* Function Name: ps2_get_y
********************************************************************************
* Returns the most current reading of the Y direction.  Only the lower 12-bits
*  contain data.
********************************************************************************/
uint16_t ps2_get_y(void)
{
  uint16_t adc_val;
  
  return adc_val;
}

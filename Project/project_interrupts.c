// Copyright (c) 2015-19, Joe Krachey
// All rights reserved.
//
// Redistribution and use in source or binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in source form must reproduce the above copyright 
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

#include "project_interrupts.h"

//*****************************************************************************
// Returns the most current direction that was pressed.
//*****************************************************************************
PS2_DIR_t ps2_get_direction(void)
{
  if(PS2_X_DATA > PS2_ADC_HIGH_THRESHOLD) 
	{
		return PS2_DIR_LEFT; 
	} 
	else if(PS2_X_DATA < PS2_ADC_LOW_THRESHOLD) 
	{
		return PS2_DIR_RIGHT;
	} 
	else if(PS2_Y_DATA > PS2_ADC_HIGH_THRESHOLD) 
	{
		return PS2_DIR_UP;
	} 
	else if (PS2_Y_DATA < PS2_ADC_LOW_THRESHOLD) 
	{
		return PS2_DIR_DOWN;
	} 
	else 
	{
		return PS2_DIR_CENTER;
	}
}


//*****************************************************************************
// TIMER0A ISR used for directional push button detection
//*****************************************************************************
void TIMER0A_Handler(void)
{
	ALERT_PUSH = true; 
	TIMER0->ICR |= TIMER_ICR_TATOCINT; 
}

//*****************************************************************************
// TIMER1 ISR used blink LED every one second
//*****************************************************************************
void TIMER1A_Handler(void)
{
	ALERT_BLINK = true; 
	TIMER1->ICR |= TIMER_ICR_TATOCINT; 
}



//*****************************************************************************
// TIMER2 ISR used to determine if space bar is hit
//*****************************************************************************
void TIMER2A_Handler(void)
{	
	int input; 
	
	input = fgetc_nb(); 
	if(input == ' ') {
		SPACE_BAR_HIT = true; 
	}
  // Clear the interrupt
	TIMER2->ICR |= TIMER_ICR_TATOCINT;
}

//*****************************************************************************
// TIMER3 ISR is used to trigger the ADC 
//*****************************************************************************
void TIMER3A_Handler(void)
{
	ADC0->PSSI = ADC_PSSI_SS2; 
	TIMER3->ICR |= TIMER_ICR_TATOCINT; 
}

//*****************************************************************************
// TIMER4A ISR checks the ADC data to determine when to move the offensive player
//*****************************************************************************
void TIMER4A_Handler(void)
{
	
	ALERT_OFFENSE = true; 
  // Clear the interrupt
	TIMER4->ICR |= TIMER_ICR_TATOCINT;
}


//*****************************************************************************
// TIMER4B ISR Defense Randomization
//*****************************************************************************
void TIMER4B_Handler(void)
{	
	static int count_d1 = 0; 

  ALERT_DEFENSE2 = true;
	ALERT_DEFENSE3 = true; 
	count_d1++; 
	if(count_d1 % 2 == 0) {
		ALERT_DEFENSE1 = true; 
	}
	
	// Clear the interrupt
	TIMER4->ICR |= TIMER_ICR_TBTOCINT;  
}
 

//*****************************************************************************
// ADC0 SS2 ISR
//*****************************************************************************
void ADC0SS2_Handler(void)
{
	// Read data from the FIFO
	PS2_X_DATA = ADC0->SSFIFO2 & 0xFFF;
	PS2_Y_DATA = ADC0->SSFIFO2 & 0xFFF;
		
	PS2_DIR = ps2_get_direction(); 
	
  // Clear the interrupt
  ADC0->ISC |= ADC_ISC_IN2;
}


//*****************************************************************************
// GPIOF Handler goes off when an interrupt occurs due to directional push 
// button being pressed from IO Expander
//*****************************************************************************
void GPIOF_Handler(void)
{
	uint8_t unnecessary_data; 
	
	//	ALERT_PUSH = true; 
	if(!ALERT_PUSH) {
		ALERT_PUSH = true; 
	} 
	GPIOF->ICR |= GPIO_ICR_GPIO_M; 
	//GPIOF->ICR |= 0x01; 
}	






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

#include "project_hardware_init.h"

void init_hardware(void)
{
	DisableInterrupts(); 
	
	// Configure LCD screen and Touch Screen
  lcd_config_gpio();
  lcd_config_screen();
  lcd_clear_screen(LCD_COLOR_BLACK);   
	ft6x06_init();
		
	// Configure serial-debug for UART0
	init_serial_debug(true, true); 
	
	// Configure Timers
	gp_timer_config_32(TIMER1_BASE, TIMER_TAMR_TAMR_PERIOD, 50000000, false, true); 		// timer 1 used to blink LED 0 every 1 second (on 1s, off 1s)
	gp_timer_config_32(TIMER2_BASE,TIMER_TAMR_TAMR_PERIOD, 1000000, false, true); 			// timer 2 used for SPACEBAR detection
	gp_timer_config_32(TIMER3_BASE,TIMER_TAMR_TAMR_PERIOD, 500000, false, true);				// timer 3 used to handle movement of offensive player
	gp_timer_config_16(TIMER4_BASE,TIMER_TAMR_TAMR_PERIOD, 50000, 10, false, true); 		// timer 4A used to trigger ADC to check the ADC for new data
	gp_timerB_config_16(TIMER4_BASE, TIMER_TBMR_TBMR_PERIOD, 50000, 10, false, true); 	// timer 4B used to handle random movement of 3 defensive lines
	
	// Configure EEPROM novolatile memory for high score
	eeprom_init(); 
	
	// Configure Port Expander for Directional Button Input 
	io_expander_init();
	configure_buttons();
	write_leds(0xFF); 
		
	// Configure Tiva Lanchpad LED and GPIOF 
	lp_io_init(); 
	
	// Configure PS2 Joystick
	ps2_initialize();
	
	EnableInterrupts(); 
	
}


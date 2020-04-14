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

#include "main.h"
#include <stdlib.h>
#include <string.h>

uint8_t high_score = 4; 
uint8_t current_score = 0; 

// Messages to display on LCD throughout various moments of the game
char win_msg[] = "You Win!";
char lose_msg[] = "GAME  OVER";
char welcome_msg1[] = "WELCOME TO";
char welcome_msg2[] = "FOOTBALL STARS";
char credit_msg1[] = "By Prasoon S";
char credit_msg2[] = "& Charles J";
char play_msg1[] = "PLAY";
char play_msg2[] = "GAME";

//*****************************************************************************
//*****************************************************************************
void DisableInterrupts(void)
{
  __asm {
         CPSID  I
  }
}

//*****************************************************************************
//*****************************************************************************
void EnableInterrupts(void)
{
  __asm {
    CPSIE  I
  }
}


//*****************************************************************************
// Function to easily print strings to the LCD
//
// parameters - x_start: where first character is placed in x-direction
// 						- y_start: where first character is placedin y-direction
// 						- print_message: string to print to LCD
// return     - none
//*****************************************************************************
void lcd_print_string(uint16_t x_start, uint16_t y_start, char *print_message) 
{
	int i; 								// for loop iterator
	char character; 			// character in string
	int index; 						// index in bitmap that corresponds to certain character
	int size; 						// size of string to print to LCD

	size = strlen(print_message); 
	
	for(i = 0; i < size; i++) {
		character = print_message[0]; 
		// if space, just move the start position of next character over 10 pixels
		if(character == ' ') {
			x_start = x_start + 10;
		} 
		// print character to the screen
		else {
			index = ((character & 31) - 1) * 48;			// logic to convert char to alphabet number position
																								// multiplied by 48 to get to correct index in bit map
			lcd_draw_image(x_start, 19, y_start, 16, &microsoftSansSerif_16ptBitmaps[index], LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
			x_start = x_start + 15;
		}
		print_message = print_message + 1; 
	}
}	


//*****************************************************************************
// Display main menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_main_menu()
{
	lcd_print_string(50, 70, welcome_msg1); 
	lcd_print_string(25, 100, welcome_msg2); 
}

int 
main(void)
{
	
	int input; 						// Input from keyboard 
	uint32_t mode;  			// 4 Modes to the game, check main.h macros to know the four modes

	init_hardware(); 			// function found in project_hardware_init.c
	
	
	print_main_menu(); 
	
	// ALL THE FOLLOWING CODE ISN'T NECESSARILY HOW THE GAME WILL GO, JUST TESTING FUNCTIONALITY OF UART SERIAL
	// DEBUG FOR PAUSE/RESUME FUNCTIONALITY AND EEPROM FUNCTIONALITY, WILL HAVE TO CHANGE TO HAVE IT IMPLEMENTED
	// WITHIN OUR GAME
	eeprom_byte_read(I2C1_BASE, HIGH_SCORE_ADDRESS, &high_score); // determine current high score to print to LCD
	//printf("%i\n\r", high_score); 
	//printf("%i\n\r", current_score);
	mode = GAME_IN_PROGRESS; 
 	while(mode != GAME_OVER) {
		input = fgetc_nb(); // Charlie, contact me if you wanna know what is going on here 
		if(input == ' ') {
			current_score++;
			if(mode == GAME_IN_PROGRESS) {
				printf("\n\rGame paused"); 
				mode = GAME_PAUSED; 
			} else if (mode == GAME_PAUSED) {
				printf("\n\rGame in progress"); 
				mode = GAME_IN_PROGRESS; 
			}
		}
		if(current_score > high_score) {
			printf("\n\rNew high score!!"); 
			eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, current_score); 
			current_score = 0; 
			mode = GAME_OVER;
		}
	}
}

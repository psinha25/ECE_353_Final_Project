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

uint8_t high_score = 0; 
uint8_t current_score = 5; 

// Messages to display on LCD throughout various moments of the game
char win_msg[] = "YOU WIN!";
char welcome_msg1[] = "WELCOME TO";
char welcome_msg2[] = "FOOTBALL STARS";
char credit_msg1[] = "BY PRASOON S";
char credit_msg2[] = "CHARLES J";
char play_msg[] = "PLAY GAME";
char high_score_msg[] = "HIGH SCORE IS"; 
char game_over_msg[] = "GAME OVER"; 
char thank_you_msg1[] = "THANK YOU";
char thank_you_msg2[] = "FOR PLAYING"; 
char your_score_msg[] = "YOUR SCORE WAS"; 
char play_again_msg[] = "PLAY AGAIN";


// Set by timer handler whenever space bar is pressed
volatile bool SPACE_BAR_HIT = false; 

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
void lcd_print_string(
	uint16_t x_start, 
	uint16_t y_start, 
	char *print_message,
	uint16_t fColor,
	uint16_t bColor,
	uint16_t font_size
) 
{
	int i; 								// for loop iterator
	char character; 			// character in string
	int index; 						// index in bitmap that corresponds to certain character
	int size; 						// size of string to print to LCD

	size = strlen(print_message); 
	//printf("%i", size); 
	for(i = 0; i < size; i++) {
		character = print_message[0]; 
		// if space, just move the start position of next character over 10 pixels
		if(character == ' ') {
			switch(font_size) {
				case 16: x_start = x_start + 8;
				case 10: x_start = x_start + 5; 
				case 0: x_start = x_start + 5; 
			}
		} 
		// print character to the screen
		else {
			switch(font_size) {
				case 16: {
					index = ((character & 31) - 1) * 48;			// logic to convert char to alphabet number position
																										// multiplied by 48 to get to correct index in bit map
					lcd_draw_image(x_start, 19, y_start, 16, &microsoftSansSerif_16ptBitmaps[index], fColor, bColor); 
					x_start = x_start + 15;
					break; 
				}
				case 10: {
					index = ((character & 31) - 1) * 20; 
					lcd_draw_image(x_start, 13, y_start, 10, &microsoftSansSerif_10ptBitmaps[index], fColor, bColor); 
					x_start = x_start + 10; 
					break;
				}
				case 0: {
					index = atoi(&character) * 10;
					lcd_draw_image(x_start, 5, y_start, 10, &microsoftSansSerif_10ptNumBitmaps[index], fColor, bColor);  
					x_start = x_start + 10;
				}
			}
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
	char score_string[100]; 
	
	lcd_print_string(50, 40, welcome_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_print_string(25, 70, welcome_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_draw_image(120, footballWidthPages, 120, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);  
	lcd_print_string(50, 160, credit_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	lcd_print_string(85, 180, credit_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(65, 230, play_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	
	// get current high score from EEPROM and print to screen
	eeprom_byte_read(I2C1_BASE, HIGH_SCORE_ADDRESS, &high_score); // determine current high score to print to LCD
	lcd_print_string(50, 270, high_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(score_string,"%i", high_score); 
	lcd_print_string(190, 270, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0);  
	
}


void print_game_over()
{
	char score_string[100]; 
	
	lcd_clear_screen(LCD_COLOR_BLACK); 
	lcd_print_string(60, 40, game_over_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_print_string(60, 70, thank_you_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_print_string(45, 100, thank_you_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_draw_image(120, footballWidthPages, 150, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(40, 180, your_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	sprintf(score_string,"%i", current_score);
	lcd_print_string(190, 180, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0);
		
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(55, 230, play_again_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 

	
}

int 
main(void)
{
	
	int input; 						// Input from keyboard 
	uint32_t mode;  			// 4 Modes to the game, check main.h macros to know the four modes
	uint16_t lcd_x; 			// x coordinate of touch screen
	uint16_t lcd_y; 			// y coordinate of touch screen
	
	init_hardware(); 			// function found in project_hardware_init.c
	
	mode = MAIN_MENU; 		
	print_main_menu(); 	
	
	while(mode == MAIN_MENU) {
		
		lcd_x = 0;
		lcd_y = 0;
	
		if (ft6x06_read_td_status())
		{
			lcd_x = ft6x06_read_x();
			lcd_y = ft6x06_read_y();
		}
		
		if((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM)) {
			printf("\n\rPlaying the game"); 
			mode = GAME_OVER; 
		}
	}
	
	while(mode == GAME_IN_PROGRESS) 
	{
			// space bar pressed, pause game
		if(SPACE_BAR_HIT) 
		{
			SPACE_BAR_HIT = false; 
			// Stay paused until space bar hit again
			while(!SPACE_BAR_HIT) {}
			SPACE_BAR_HIT = false; 
		}
		// Actual game logic over here!!! 
		printf("\n\rGame in progress/resumed"); 
	}
	
	print_game_over(); 
	while(mode == GAME_OVER) {
		if(current_score > high_score) {
			printf("NEW HIGH SCORE!"); 
			eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, current_score); 
			current_score = 0;
		}
		
		lcd_x = 0;
		lcd_y = 0;
	
		if (ft6x06_read_td_status())
		{
			lcd_x = ft6x06_read_x();
			lcd_y = ft6x06_read_y();
		}
		
		if((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM)) {
			printf("\n\rPlaying the game"); 
			mode = GAME_OVER; 
		}
	} 
	
	
	
	
	
	
	
	
	
	
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

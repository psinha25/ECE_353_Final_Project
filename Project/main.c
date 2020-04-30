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

uint8_t high_score = 0; 				// will be read from EEPROM
uint8_t current_score = 10; 		// tracked score of current player

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
char level_up_msg[] = "LEVELED UP"; 
char start_new_level_msg[] = "NEW LEVEL"; 
char congrats_msg[] = "CONGRATULATION"; 
char lives_msg[] = "LIVES"; 
char reset_high_score[] = "RESET HIGH SCORE"; 

// Set by Timer2 Handler whenever space bar is pressed
volatile bool SPACE_BAR_HIT = false; 
bool PAUSED = false; 

// Set by timer 1A Handler every 1 second to blink LED
volatile bool ALERT_BLINK = true; 
WS2812B_t LEDs[1]; 

// Global variables for control of offensive player via joystick
volatile uint16_t OFFENSE_X_COORD = 35;
volatile uint16_t OFFENSE_Y_COORD = 35;
volatile bool ALERT_OFFENSE = true;

// Global variables line 1 defense - randomized
volatile uint16_t DEFENSE_1X_COORD = 120; // 120
volatile uint16_t DEFENSE_1Y_COORD = 160; // 215
volatile bool ALERT_DEFENSE1 = true; 
volatile int ALERT_D1_INT = 0; 

// Global variables line 2 defense - randomized
volatile uint16_t DEFENSE_2X_COORD = 120; // 120
volatile uint16_t DEFENSE_2Y_COORD = 215; // 160
volatile bool ALERT_DEFENSE2 = true; 

// Global variables line 3 defense - randomized
volatile uint16_t DEFENSE_3X_COORD = 120;	// 120
volatile uint16_t DEFENSE_3Y_COORD = 105; // 105
volatile bool ALERT_DEFENSE3 = true; 

// Global variables for directional push button detection
volatile bool ALERT_PUSH = false; 

// Variables for defense randomization
static const uint16_t START_STATE = 0xACE7u;
static const uint16_t MOVE_AMOUNT[] = {5, 10, 15, 20, 25, 30, 35};
static const PS2_DIR_t MOV_DIR[] = {PS2_DIR_UP, PS2_DIR_DOWN, PS2_DIR_LEFT, PS2_DIR_RIGHT};

volatile uint16_t PS2_X_DATA = 0;
volatile uint16_t PS2_Y_DATA = 0;
volatile PS2_DIR_t PS2_DIR = PS2_DIR_CENTER;

static PS2_DIR_t current_direction_d1; 
static uint32_t move_count_D1 = 0; 
	
static PS2_DIR_t current_direction_d2;
static uint32_t move_count_D2 = 0;
	
static PS2_DIR_t current_direction_d3; 
static uint32_t move_count_D3 = 0; 

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
// Generates a random number
// https://en.wikipedia.org/wiki/Linear-feedback_shift_register  -- DO NOT MODIFY
//*****************************************************************************
uint16_t generate_random_number(
)
{   
    static uint16_t lfsr = START_STATE;
    uint16_t bit;
    bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return lfsr;
}

//*****************************************************************************
// Generates the the new direction and number of pixels
//*****************************************************************************
PS2_DIR_t get_new_direction(PS2_DIR_t curr_direction)
{
     PS2_DIR_t new_direction;
    do
    {
        new_direction = MOV_DIR[generate_random_number()%4];
    }while (new_direction == curr_direction);
    
    return new_direction;
}

//*****************************************************************************
// Generates the the new direction and number of pixels
//*****************************************************************************
uint16_t get_new_move_count(void)
{
    return MOVE_AMOUNT[generate_random_number()%8];
}


//*****************************************************************************
// Function to easily print strings to the LCD
//
// parameters - x_start: where first character is placed in x-direction
// 						- y_start: where first character is placedin y-direction
// 						- print_message: string to print to LCD
//						- fColor: color of the words to print
// 						- bColor: background color of the words to print
//						- font_size: 3 options (16 point font, 10 point font, or print numbers)
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
	for(i = 0; i < size; i++) {
		character = print_message[0]; 
		// if a space, just move the start position of next character over 
		// based on font size specified
		if(character == ' ') {
			switch(font_size) {
				case 16: x_start = x_start + 8;
				case 10: x_start = x_start + 5; 
				case 0: x_start = x_start + 5; 
			}
		} 
		// print character to the screen based on font size
		else {
			switch(font_size) {
				case 16: {
					index = ((character & 31) - 1) * 48;			// logic to convert char to alphabet number position
																										// multiplied by 48 to get to correct index in 16 point font bit map
					lcd_draw_image(x_start, 19, y_start, 16, &microsoftSansSerif_16ptBitmaps[index], fColor, bColor); 
					x_start = x_start + 15;
					break; 
				}
				case 10: {
					index = ((character & 31) - 1) * 20; 			// logic to convert char to alphabet number position
																										// multiplied by 20 to get to correct index in 10 point font bit map
					lcd_draw_image(x_start, 13, y_start, 10, &microsoftSansSerif_10ptBitmaps[index], fColor, bColor); 
					x_start = x_start + 10; 
					break;
				}
				case 0: {
					printf("\n\r size of message is %i", size); 
					index = atoi(&character) * 10;						// logic to convert number given as a string to correct index
																										// in the number bit map
					lcd_draw_image(x_start, 5, y_start, 10, &microsoftSansSerif_10ptNumBitmaps[index], fColor, bColor);  
					x_start = x_start + 10;
				}
			}
		}
		print_message = print_message + 1; 							// move to next character in the given string to print
	}
}	


//*****************************************************************************
// Display main menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_main_menu(void)
{
	char score_string[100]; 			// store the current high score as a string
	
	lcd_print_string(50, 25, welcome_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_print_string(25, 55, welcome_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_draw_image(120, footballWidthPages, 95, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);  
	lcd_print_string(50, 120, credit_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	lcd_print_string(85, 140, credit_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	
	lcd_draw_image(120, boundary_Reset_High_Score_Width, 180, boundary_Reset_High_Score_Height, boundary_Reset_High_Score, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(45, 180, reset_high_score, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(65, 230, play_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	
	// get current high score from EEPROM and print to screen
	eeprom_byte_read(I2C1_BASE, HIGH_SCORE_ADDRESS, &high_score); 											// determine current high score to print to LCD
	printf("\n\rhigh score is %i: ", high_score); 
	lcd_print_string(50, 270, high_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(score_string,"%i", high_score); 																						// convert high score to a string to print
	printf("\n\rscore string is %s: ", score_string); 
	lcd_print_string(190, 270, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0);  
	
}

void print_level_up()
{
	char score_string[100]; 
	lcd_print_string(50, 40, level_up_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_print_string(20, 70, congrats_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_draw_image(120, footballWidthPages, 120, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(40, 160, your_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(score_string, "%i", current_score); 
	lcd_print_string(190, 160, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0); 
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
	lcd_print_string(65, 230, start_new_level_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
}

//*****************************************************************************
// Display game over menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_game_over(void)
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

void print_lives(uint8_t lives_lost, uint8_t level_reached)
{
	char lives_string[100]; 
	uint8_t lives_remaining = level_reached + 1 - lives_lost; 
	
	lcd_print_string(170, 12, lives_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(lives_string, "%i", lives_remaining); 
	lcd_print_string(225, 12, lives_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0); 
}

//*****************************************************************************
// Defense line 1 has certain boundaries that it can move in. Determine if 
// moving the player in specified direction will result in them moving past 
// their boundary
//*****************************************************************************
bool defense_L1_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
)
{
	
	bool edge_contacted = false; 
	
	// Based on direction in which to move image by 1 pixel
	// determine if current location of image is at any edge
	switch (direction) {
		case PS2_DIR_DOWN: 
			if(y_coord + (image_height/2) == 185)
				edge_contacted = true; 
			break; 
		case PS2_DIR_UP: 
			if(y_coord - (image_height/2) == 135)
				edge_contacted = true; 
			break; 
		case PS2_DIR_LEFT: 
			if(x_coord - (image_width / 2) == 78)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 162)
				edge_contacted = true; 
			break; 
		default: break; 
	}
	return edge_contacted; 
}


//*****************************************************************************
// Defense line 2 has certain boundaries that it can move in. Determine if 
// moving the player in specified direction will result in them moving past 
// their boundary
//*****************************************************************************
bool defense_L2_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
)
{
	
	bool edge_contacted = false; 
	
	// Based on direction in which to move image by 1 pixel
	// determine if current location of image is at any edge
	switch (direction) {
		case PS2_DIR_DOWN: 
			if(y_coord + (image_height/2) == 240)
				edge_contacted = true; 
			break; 
		case PS2_DIR_UP: 
			if(y_coord - (image_height/2) == 190)
				edge_contacted = true; 
			break; 
		case PS2_DIR_LEFT: 
			if(x_coord - (image_width / 2) == 78)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 162)
				edge_contacted = true; 
			break; 
		default: break; 
	}
	return edge_contacted; 
}

//*****************************************************************************
// Defense line 3 has certain boundaries that it can move in. Determine if 
// moving the player in specified direction will result in them moving past 
// their boundary
//*****************************************************************************
bool defense_L3_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
)
{
	
	bool edge_contacted = false; 
	
	// Based on direction in which to move image by 1 pixel
	// determine if current location of image is at any edge
	switch (direction) {
		case PS2_DIR_DOWN: 
			if(y_coord + (image_height/2) == 130)
				edge_contacted = true; 
			break; 
		case PS2_DIR_UP: 
			if(y_coord - (image_height/2) == 80)
				edge_contacted = true; 
			break; 
		case PS2_DIR_LEFT: 
			if(x_coord - (image_width / 2) == 78)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 162)
				edge_contacted = true; 
			break; 
		default: break; 
	}
	return edge_contacted; 
}


//*****************************************************************************
// Determines if any part of the offensive player would be off the screen 
// if the player is moved in the specified direction.
//*****************************************************************************
bool contact_edge(
    volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
)
{
	
	bool edge_contacted = false; 
	
	// Based on direction in which to move image by 1 pixel
	// determine if current location of image is at any edge
	switch (direction) {
		case PS2_DIR_DOWN: 
			if(y_coord + (image_height/2) == 319)
				edge_contacted = true; 
			break; 
		case PS2_DIR_UP: 
			if(y_coord - (image_height/2) == 0)
				edge_contacted = true; 
			break; 
		case PS2_DIR_LEFT: 
			if(x_coord - (image_width / 2) == 0)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 239)
				edge_contacted = true; 
			break; 
		default: break; 
	}
	
	return edge_contacted; 
	
}


//*****************************************************************************
// Moves the image by one pixel in the provided direction.  The second and 
// third parameter should modify the current location of the image (pass by
// reference)
//*****************************************************************************
void move_image(
        volatile PS2_DIR_t direction,
        volatile uint16_t *x_coord, 
        volatile uint16_t *y_coord, 
        uint8_t image_height, 
        uint8_t image_width,
				bool line_stopped
)
{
	// Move the image one pixel based on the provided direction
	// only if game isn't paused, otherwise when unpaused
	// new image will be drawn in a different location and mess up 
	// flow of movement of the image
	if(!PAUSED && !line_stopped)
	{
		switch(direction) {
		case PS2_DIR_DOWN: 
			*y_coord = *y_coord + 1; 
			break; 
		case PS2_DIR_UP:  
			*y_coord = *y_coord - 1; 
			break;
		case PS2_DIR_LEFT: 
			*x_coord = *x_coord - 1; 
			break; 
		case PS2_DIR_RIGHT:  	
			*x_coord = *x_coord + 1;
			break; 
		default: break; 
	 }
	}
}

void handle_d1(bool line_stopped)
{
	if(move_count_D1 == 0) {
		current_direction_d1 = get_new_direction(current_direction_d1);
		move_count_D1 = get_new_move_count(); 
	}
	
	// Get new direction and move count if next_move results in contact with the LCD screen edges
	// While loop ensures that we check whether the new direction give to us after one pass through
	// does not result in the ship being moved past the boundary of the LCD screen
	while(defense_L1_boundary_reached(current_direction_d1, DEFENSE_1X_COORD, DEFENSE_1Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d1 = get_new_direction(current_direction_d1);
		move_count_D1 = get_new_move_count();
	}

	move_count_D1--; 
	move_image(current_direction_d1, &DEFENSE_1X_COORD, &DEFENSE_1Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

void handle_d2(bool line_stopped)
{
	
	if(move_count_D2 == 0) {
		current_direction_d2 = get_new_direction(current_direction_d2);
		move_count_D2 = get_new_move_count(); 
	}

	while(defense_L2_boundary_reached(current_direction_d2, DEFENSE_2X_COORD, DEFENSE_2Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d2 = get_new_direction(current_direction_d2);
		move_count_D2 = get_new_move_count();
	}	

	move_count_D2--; 
	move_image(current_direction_d2, &DEFENSE_2X_COORD, &DEFENSE_2Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

void handle_d3(bool line_stopped) 
{
	if(move_count_D3 == 0) {
		current_direction_d3 = get_new_direction(current_direction_d3);
		move_count_D3 = get_new_move_count(); 
	}

	while(defense_L3_boundary_reached(current_direction_d3, DEFENSE_3X_COORD, DEFENSE_3Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d3 = get_new_direction(current_direction_d3);
		move_count_D3 = get_new_move_count();
	}

	move_count_D3--; 
	move_image(current_direction_d3, &DEFENSE_3X_COORD, &DEFENSE_3Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

void update_lost_life(uint8_t level_reached, bool *cleared, uint8_t *lives_lost)
{
	
	bool life_lost = false; 
	
  if(!cleared[0] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 1"); 
		life_lost = true; 
	}
	else if(!cleared[2] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD + 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD + 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 2"); 
		life_lost = true; 
	}
	
	else if(!cleared[1] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD - 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD - 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 2"); 
		life_lost = true;   
	}
	
	else if((level_reached >= 2) && !cleared[3] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 4"); 
		life_lost = true;   
	}
	else if((level_reached >= 2) && !cleared[5] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD + 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD + 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true; 
	printf("\n\rIn if 5"); 		
		life_lost = true;   
	}
	
	else if((level_reached >= 2) && !cleared[4] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD - 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD - 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 6"); 
		life_lost = true;   
	}
	
	else if((level_reached >= 3) && !cleared[6] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true; 
		printf("\n\rIn if 7"); 		
		life_lost = true; 
	}
	else if((level_reached >= 3) && !cleared[8] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD + 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD + 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true; 
		printf("\n\rIn if 8"); 		
		life_lost = true;   
	}
	
	else if((level_reached >= 3) && !cleared[7] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD - 76 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD - 76 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{
		// return true;  
		printf("\n\rIn if 9"); 
		life_lost = true;   
	}
	
	// printf("\n\rLife lost boolean is %d", life_lost);
	
	if(life_lost) {
		*lives_lost = *lives_lost + 1; 
		lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
								 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
								 offensive_PlayerBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK); 
		print_lives(*lives_lost, level_reached); 
		OFFENSE_X_COORD = 50; 
		OFFENSE_Y_COORD = 50; 
		lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
								 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
								 offensive_PlayerBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
		printf("\n\rLost a life: %i", *lives_lost); 
	}
	
//	return false; 
	
}

void update_io_leds(uint8_t num_presses)
{
	//printf("in update_io_leds"); 
	switch (num_presses){
		case 1: 
			write_leds(0x3F); 
			break; 
		case 2:
			write_leds(0x0F); 
			break; 
		case 3: 
			write_leds(0x03); 
			break; 
		case 4: 
			write_leds(0x00); 
			break; 
		default:
			write_leds(0x00);
	}
}

void reset_level(uint8_t level_reached, bool *d_line_stop, bool *dplayer_clear, bool *cleared, uint8_t *num_line_left, uint8_t *num_dplayers_left)
{
	
	uint8_t i; 
	for(i = 0; i < 3; i++) {
		d_line_stop[i] = false; 
	}
	for(i = 0; i < 9; i++) {
		dplayer_clear[i] = false;
		cleared[i] = false; 
	}
	if(level_reached == 1) {
		*num_line_left = 1; 
		*num_dplayers_left = 3; 
	}
	else if(level_reached == 2) {
		*num_line_left = 2; 
		*num_dplayers_left = 6; 
	}
	else if(level_reached == 3) {
		*num_line_left = 2; 
		*num_dplayers_left = 9; 
	}		
	
}

bool check_game_over(uint8_t lives_lost, uint8_t level_reached) 
{
	if(lives_lost == 2 && level_reached == 1) {
		return true;  
	}
	else if(lives_lost == 3 && level_reached == 2) {
		return true; 
	}
	else if(lives_lost == 4 && level_reached == 3) {
		return true; 
	}
	return false; 
}


int main(void)
{
	
	uint32_t mode;  							// 4 Modes to the game, check main.h macros to know the four modes
	uint16_t lcd_x; 							// x coordinate of touch screen
	uint16_t lcd_y; 							// y coordinate of touch screen
	bool d_line_stop[3];  				// if set to true, that line doesn't move (i.e. if d_line_stop[0] == true, line 0 doesn't move)
	uint8_t num_line_left; 				
	uint8_t num_line_stopped; 
	uint8_t line_index; 
	bool dplayer_clear[9]; 
	uint8_t num_dplayers_left;
	uint8_t num_dplayers_cleared; 
	uint8_t dplayer_index; 
	bool cleared[9]; 							// set once we have cleared a defensive player from the screen so that we don't continue drawing moving black rectangles throughout the screen
	uint8_t level_reached; 				// level the user is on - 3 different levels, increasing difficulty 
	static uint8_t lives_lost; 
	static uint8_t num_presses; 
	uint8_t button_data; 
	DEBOUNCE_STATES state_up = DEBOUNCE_ONE;
	DEBOUNCE_STATES state_down = DEBOUNCE_ONE;
	DEBOUNCE_STATES state_left = DEBOUNCE_ONE;
	DEBOUNCE_STATES state_right = DEBOUNCE_ONE;
	bool up_pressed; 
	bool down_pressed;
	bool left_pressed; 
	bool right_pressed;
	bool light_on; 
	
	init_hardware(); 							// function found in project_hardware_init.c  	
			
	printf("\n\r**************** WELCOME TO FOOTBALL STARS!! ****************"); 

		
	mode = MAIN_MENU; 		
	print_main_menu();
	num_presses = 0; 
	printf("\n\rYour score is %i", current_score); 
	while(1) {
		
		while(mode == MAIN_MENU) {
			// printf("\n\rin main menu"); 
			
			lcd_x = 0;
			lcd_y = 0;
		
			if (ft6x06_read_td_status())
			{
				// printf("\n\r in read status"); 
				if(debounce_fsm_lcd(false)) {
					printf("\n\r debounce_fsm_lcd() was true");
					lcd_x = ft6x06_read_x();
					lcd_y = ft6x06_read_y();
				}
			}
			else {
				debounce_fsm_lcd(true); 
			}
			
			if((lcd_x > RESET_LEFT) && (lcd_x < RESET_RIGHT) && (lcd_y > RESET_TOP) && (lcd_y < RESET_BOTTOM)) {
				eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, 0); 
				lcd_clear_screen(LCD_COLOR_BLACK); 
				print_main_menu(); 
			}
			
			// Wait for user to touch the play game box to start playing the game
			else if((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM)) {
				printf("\n\rPlaying the game"); 
				mode = GAME_IN_PROGRESS; 
				num_dplayers_left = 3; 
				num_line_left = 1; 
				level_reached = 1; 
				lcd_clear_screen(LCD_COLOR_BLACK);
				lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
											 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
											 offensive_PlayerBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
				print_lives(lives_lost, level_reached); 
			}
		}
		
		while(mode == GAME_IN_PROGRESS) 
		{
			// Pause/Resume game functionality when space bar is hit
			if(SPACE_BAR_HIT) 
			{
				printf("\n\rGame paused"); 
				PAUSED = true; 
				SPACE_BAR_HIT = false; 
				while(!SPACE_BAR_HIT) {}						// Stay paused until space bar hit again
				SPACE_BAR_HIT = false; 
				PAUSED = false; 
				printf("\n\rGame in progress/resumed");
			}
			
			// Handling push button being pressed - only left and down button do anything
			// User gets to push the buttons 4 times per level, each time one of the two
			// buttons is pushed, the number of TURNED ON LEDs on the IO Expander decrease
			// by two. All the LEDs being turned off indicates to user that they have 
			// pushed the directional push buttons four times during that level. Reset for
			// every level.
			if(ALERT_PUSH) 
			{
				get_button_data(&button_data); 
				up_pressed = debounce_fsm(&state_up, button_data & (1 << UP_BUTTON_PIN)); 
				down_pressed = debounce_fsm(&state_down, button_data & (1 << DOWN_BUTTON_PIN)); 
				left_pressed = debounce_fsm(&state_left, button_data & (1 << LEFT_BUTTON_PIN)); 
				right_pressed = debounce_fsm(&state_right, button_data & (1 << RIGHT_BUTTON_PIN)); 
				if(num_presses < 4) 
				{
					// When down is pressed, a random defensive line stops moving and stays in 
					// place on the screen. 
					if(down_pressed) 
					{
						printf("\n\r down pressed"); 

						line_index = generate_random_number() % num_line_left; 
						while(d_line_stop[line_index] && (num_line_stopped < num_line_left)) 
						{
							line_index = generate_random_number() % num_line_left; 
						}
						d_line_stop[line_index] = true;
						num_line_stopped++; 
						num_presses++; 
						printf("\n\rnumber of presses is %i", num_presses);
						update_io_leds(num_presses); 																		// decrease the number of LEDs on by two
					}
					// When left is pressed, a random defensive player is cleared from the screen. 
					// If a defensive line has been stopped, a defensive player on that line could
					// be randomly chosen to get cleared, but because the line the player is on
					// is already stopped, the player won't get cleared.
					else if (left_pressed)
					{
						printf("\n\rleft pressed"); 
						dplayer_index = generate_random_number() % num_dplayers_left;		// can't go straight into while loop otherwise
																																						// first run of the level, the player at index 0
																																						// will be removed from the screen every time
						while(dplayer_clear[dplayer_index] && (num_dplayers_cleared < num_dplayers_left)) 
						{
							dplayer_index = generate_random_number() % num_dplayers_left; 
						}
						dplayer_clear[dplayer_index] = true; 
						num_dplayers_cleared++; 
						num_presses++; 
						printf("\n\rnumber of presses is %i", num_presses); 
						update_io_leds(num_presses); 																		// decrease the number of LEDs on by two
					}
				}
				//ALERT_PUSH = false; 
			}
			
			// Handle blinking LED 0 every 1 second
			if(ALERT_BLINK)
			{
				if(!light_on) 
				{
					// printf("\n\rIn setting");
					lp_io_set_pin(GREEN_BIT);
					light_on = true; 
				}
				else 
				{
					// printf("\n\rIn clearing"); 
					lp_io_clear_pin(GREEN_BIT);
					light_on = false; 
				}
				ALERT_BLINK = false;
			}
			
			// Actual game logic over here!!! 
			if(ALERT_OFFENSE)
			{
				ALERT_OFFENSE = false; 
				if(!contact_edge(PS2_DIR, OFFENSE_X_COORD, OFFENSE_Y_COORD, offensive_PlayerHeightPixels, offensive_PlayerWidthPixels)) 
				{
					move_image(PS2_DIR, &OFFENSE_X_COORD, &OFFENSE_Y_COORD, offensive_PlayerHeightPixels, offensive_PlayerWidthPixels, false);
				}
				
				if(PS2_DIR != PS2_DIR_CENTER && PS2_DIR != PS2_DIR_INIT) 
				{
					lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
											 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
											 offensive_PlayerBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
				}
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
					printf("\n\rGame is over - ALERT_OFFENSE"); 
					mode = GAME_OVER; 
					break; 
				}
			}
			
			if(ALERT_DEFENSE1 && (level_reached >= 1))
			{
				ALERT_DEFENSE1 = false; 
				handle_d1(d_line_stop[0]); 
				
				if(!d_line_stop[0]) 
				{
					if(!dplayer_clear[0]) 
					{	
						lcd_draw_image(DEFENSE_1X_COORD, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[0]) 
					{
						cleared[0] = true; 
						lcd_draw_image(DEFENSE_1X_COORD, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK); 
					}
					if(!dplayer_clear[1]) 
					{
						lcd_draw_image(DEFENSE_1X_COORD - 85, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[1]) 
					{
						cleared[1] = true; 
						lcd_draw_image(DEFENSE_1X_COORD - 85, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[2]) 
					{
						lcd_draw_image(DEFENSE_1X_COORD + 85, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[2]) 
					{
						cleared[2] = true; 
						lcd_draw_image(DEFENSE_1X_COORD + 85, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
			
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
					printf("\n\rGame is over - ALERT_DEFENSE1"); 
					mode = GAME_OVER; 
					break; 
				}
			}
			
			if(ALERT_DEFENSE2 && (level_reached >= 2))
			{
				ALERT_DEFENSE2 = false; 
				handle_d2(d_line_stop[1]); 	
				
				if(!d_line_stop[1]) 
				{
					if(!dplayer_clear[3]) 
					{
						lcd_draw_image(DEFENSE_2X_COORD, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[3])
					{
						cleared[3] = true; 
						lcd_draw_image(DEFENSE_2X_COORD, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[4]) 
					{
						lcd_draw_image(DEFENSE_2X_COORD - 76, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[4]) 
					{
						cleared[4] = true; 
						lcd_draw_image(DEFENSE_2X_COORD - 76, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[5]) 
					{
						lcd_draw_image(DEFENSE_2X_COORD + 76, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[5])
					{
						cleared[5] = true; 
						lcd_draw_image(DEFENSE_2X_COORD + 76, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
				
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
					printf("\n\rGame is over - ALERT_OFFENSE"); 
					mode = GAME_OVER; 
					break; 
				}
			}
			
			if(ALERT_DEFENSE3 && (level_reached >= 3)) 
			{
				ALERT_DEFENSE3 = true; 
				handle_d3(d_line_stop[2]); 
				if(!d_line_stop[2]) {
					if(!dplayer_clear[6]) 
					{
						lcd_draw_image(DEFENSE_3X_COORD, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					} 
					else if(!cleared[6])
					{
						cleared[6] = true; 
						lcd_draw_image(DEFENSE_3X_COORD, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);	
					}
					if(!dplayer_clear[7])
					{
						lcd_draw_image(DEFENSE_3X_COORD - 76, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[7]) 
					{
						cleared[7] = true; 
						lcd_draw_image(DEFENSE_3X_COORD - 76, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[8])
					{
						lcd_draw_image(DEFENSE_3X_COORD + 76, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[8])
					{
						cleared[8] = true; 
						lcd_draw_image(DEFENSE_3X_COORD + 76, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
					printf("\n\rGame is over - ALERT_OFFENSE"); 
					mode = GAME_OVER; 
					break; 
				}
			}	
				
			if(OFFENSE_Y_COORD >= 290) 
			{
				mode = LEVELED_UP; 
			}
		}	
		
		while(mode == LEVELED_UP) 
		{
			
			if(num_presses > 4)
				current_score = current_score + 3; 
			else
				current_score = current_score +  7 - num_presses; 
			
			if(current_score - lives_lost < 0)
				current_score = 0; 
			
			lcd_clear_screen(LCD_COLOR_BLACK);
			print_level_up(); 
			level_reached++; 
			lives_lost = 0; 
			printf("\n\rleveled up!");
			OFFENSE_X_COORD = 35; 
			OFFENSE_Y_COORD = 35;
			for(line_index = 0; line_index < 3; line_index++) 
			{
				d_line_stop[line_index] = false; 
			}
			
			for(dplayer_index = 0; dplayer_index < 9; dplayer_index++) 
			{
				dplayer_clear[dplayer_index] = false;
				cleared[dplayer_index] = false; 
			}
			
			if(level_reached == 2) 
			{
				num_line_left = 2; 
				num_dplayers_left = 6; 
			}
			else if(level_reached == 3) 
			{
				num_line_left = 2; 
				num_dplayers_left = 9; 
			}
			else if(level_reached > 3) 
			{
				lcd_clear_screen(LCD_COLOR_BLACK);
				mode = GAME_OVER;
				break; 
			}

			
			lcd_x = 0;
			lcd_y = 0;
			
			while(!((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM))) 
			{
				if (ft6x06_read_td_status())
				{
					lcd_x = ft6x06_read_x();
					lcd_y = ft6x06_read_y();
				}
			}
			printf("\n\rPlaying the game"); 
			mode = GAME_IN_PROGRESS; 
			write_leds(0xFF); 
			num_presses = 0; 
			lcd_clear_screen(LCD_COLOR_BLACK);
			print_lives(lives_lost, level_reached); 
			lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
										 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
										 offensive_PlayerBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
		}
			
		
		while(mode == GAME_OVER) 
		{
			print_game_over(); 
			if(current_score > high_score) 
			{
				printf("NEW HIGH SCORE!"); 
				eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, current_score); 
			}
			
			current_score = 0;
			lcd_x = 0;
			lcd_y = 0;
			
			while(!((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM))) 
			{
				if (ft6x06_read_td_status())
				{
					if(debounce_fsm_lcd(false)) 
					{
						printf("\n\r debounce_fsm_lcd() was true");
						lcd_x = ft6x06_read_x();
						lcd_y = ft6x06_read_y();
					}
				}
				else {
					debounce_fsm_lcd(true); 
				}
			} 
			mode = MAIN_MENU;
			OFFENSE_X_COORD = 35; 
			OFFENSE_Y_COORD = 35;
			for(line_index = 0; line_index < 3; line_index++) 
			{
				d_line_stop[line_index] = false; 
			}
			
			for(dplayer_index = 0; dplayer_index < 9; dplayer_index++) 
			{
				dplayer_clear[dplayer_index] = false;
				cleared[dplayer_index] = false; 
			}
			num_presses = 0; 
			lives_lost = 0;
			printf("\n\r**************** WELCOME TO FOOTBALL STARS!! ****************"); 
			lcd_clear_screen(LCD_COLOR_BLACK); 
			print_main_menu();
		} 
	}
}

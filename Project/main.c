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

uint8_t high_score = 0; 			// will be read from EEPROM
uint8_t current_score = 0; 		// tracked score of current player

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
volatile uint16_t DEFENSE_1X_COORD = 120; 
volatile uint16_t DEFENSE_1Y_COORD = 160; 
volatile bool ALERT_DEFENSE1 = true; 
volatile int ALERT_D1_INT = 0; 

// Global variables line 2 defense - randomized
volatile uint16_t DEFENSE_2X_COORD = 120; 
volatile uint16_t DEFENSE_2Y_COORD = 215; 
volatile bool ALERT_DEFENSE2 = true; 

// Global variables line 3 defense - randomized
volatile uint16_t DEFENSE_3X_COORD = 120;
volatile uint16_t DEFENSE_3Y_COORD = 105; 
volatile bool ALERT_DEFENSE3 = true; 

// Global variables for directional push button detection
volatile bool ALERT_PUSH = false; 

// Variables for defense randomization
static const uint16_t START_STATE = 0xACE7u;
static const uint16_t MOVE_AMOUNT[] = {5, 10, 15, 20, 25, 30, 35};
static const PS2_DIR_t MOV_DIR[] = {PS2_DIR_UP, PS2_DIR_DOWN, PS2_DIR_LEFT, PS2_DIR_RIGHT};

// Variables for offense coordinates and direction, controlled by joystick
volatile uint16_t PS2_X_DATA = 0;
volatile uint16_t PS2_Y_DATA = 0;
volatile PS2_DIR_t PS2_DIR = PS2_DIR_CENTER;

// Variables for defense line 1 coordinates and direction - randomized
static PS2_DIR_t current_direction_d1; 
static uint32_t move_count_D1 = 0; 

// Variables for defense line 2 coordinates and direction - randomized
static PS2_DIR_t current_direction_d2;
static uint32_t move_count_D2 = 0;

// Variables for defense line 3 coordinates and direction - randomized
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
	
	// Printing messages and drawing a football
	lcd_print_string(50, 25, welcome_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_print_string(25, 55, welcome_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16);
	lcd_draw_image(120, footballWidthPages, 95, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);  
	lcd_print_string(50, 120, credit_msg1, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	lcd_print_string(85, 140, credit_msg2, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	
	// Reset boundary and message
	lcd_draw_image(120, boundary_Reset_High_Score_Width, 180, boundary_Reset_High_Score_Height, boundary_Reset_High_Score, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(45, 180, reset_high_score, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	
	// Play game boundary and message
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(65, 230, play_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	
	// get current high score from EEPROM and print to screen
	eeprom_byte_read(I2C1_BASE, HIGH_SCORE_ADDRESS, &high_score); 											// read from EEPROM to display current high score
	lcd_print_string(50, 270, high_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(score_string,"%i", high_score); 																						// convert high score to a string to print
	lcd_print_string(190, 270, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0);  
	
}

//*****************************************************************************
// Display level up menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_level_up()
{
	char score_string[100]; 
	
	lcd_print_string(50, 40, level_up_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_print_string(20, 70, congrats_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 
	lcd_draw_image(120, footballWidthPages, 120, footballHeightPixels, footballBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	
	// Display current score after a level to the user
	lcd_print_string(40, 160, your_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(score_string, "%i", current_score); 
	lcd_print_string(190, 160, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0); 
	
	// New level message and boundary
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
	
	// Display final score after the game is over
	lcd_print_string(40, 180, your_score_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10);
	sprintf(score_string,"%i", current_score);
	lcd_print_string(190, 180, score_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0);
	
	// Play Again boundary and message
	lcd_draw_image(120, boundary_Play_GameWidthPixels, 230, boundary_Play_GameHeightPixels, boundary_Play_GameBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK); 
	lcd_print_string(55, 230, play_again_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 16); 

}

//*****************************************************************************
// Display lives left during game mode to the LCD screen by calling 
// lcd_print_string() and lcd_draw_image()
//
// parameters - lives_lost: number of lives lost
// 						- level_reached: logic used to determine number of lives to print
// return     - none
//*****************************************************************************
void print_lives(uint8_t lives_lost, uint8_t level_reached)
{
	char lives_string[100]; 
	uint8_t lives_remaining = level_reached + 1 - lives_lost; 		// logic to determine number of lives left
	
	lcd_print_string(170, 12, lives_msg, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 10); 
	sprintf(lives_string, "%i", lives_remaining); 
	lcd_print_string(225, 12, lives_string, LCD_COLOR_WHITE, LCD_COLOR_BLACK, 0); 
}

//*****************************************************************************
// Defense line 1 has certain boundaries that it can move in. Determine if 
// moving the player in specified direction will result in them moving past 
// their boundary
// 
// parameters - direction: direction to move the defensve line
// 						- x_coord and y_cood: coordinates of middle player on defensive line
// 						- image_height: height of the defensive players
// 						- image_width: width of the defensive players
// return			- true if boundary is reached, false otherwise
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
			if(x_coord - (image_width / 2) == 90)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 148)
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
// parameters - direction: direction to move the defensve line
// 						- x_coord and y_cood: coordinates of middle player on defensive line
// 						- image_height: height of the defensive players
// 						- image_width: width of the defensive players
// return			- true if boundary is reached, false otherwise
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
			if(x_coord - (image_width / 2) == 90)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 148)
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
// parameters - direction: direction to move the defensve line
// 						- x_coord and y_cood: coordinates of middle player on defensive line
// 						- image_height: height of the defensive players
// 						- image_width: width of the defensive players
// return			- true if boundary is reached, false otherwise
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
			if(x_coord - (image_width / 2) == 90)
				edge_contacted = true;
			break; 
		case PS2_DIR_RIGHT: 
			if(x_coord + (image_width/2) == 148)
				edge_contacted = true; 
			break; 
		default: break; 
	}
	return edge_contacted; 
}


//*****************************************************************************
// Determines if any part of the offensive player would be off the screen 
// if the player is moved in the specified direction.
// parameters - direction: direction to move the offensive player
// 						- x_coord and y_cood: coordinates of offensive player
// 						- image_height: height of the offensive player
// 						- image_width: width of the ofensive players
// return			- true if edge of boundary reached, false otherwise
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

//*****************************************************************************
// Handles movements of the defensive line one, called from main() when
// ALERT_DEFENSE1 is set by timer
// 
// parameters  - 	line_stopped: indicates if the line has been stopped, passed 
// 															to move_image() function
// return 		 - 	none
//*****************************************************************************
void handle_d1(bool line_stopped)
{
	if(move_count_D1 == 0) {
		current_direction_d1 = get_new_direction(current_direction_d1);
		move_count_D1 = get_new_move_count(); 
	}
	
	// Get new direction and move count if next_move results in contact with defense boundary
	// While loop ensures that we check whether the new direction give to us after one pass through
	// does not result in the defense line moving pass its boundary
	while(defense_L1_boundary_reached(current_direction_d1, DEFENSE_1X_COORD, DEFENSE_1Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d1 = get_new_direction(current_direction_d1);
		move_count_D1 = get_new_move_count();
	}

	move_count_D1--; 
	move_image(current_direction_d1, &DEFENSE_1X_COORD, &DEFENSE_1Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

//*****************************************************************************
// Handles movements of the defensive line 2, called from main() when
// ALERT_DEFENSE2 is set by timer
// 
// parameters  - 	line_stopped: indicates if the line has been stopped, passed 
// 															to move_image() function
// return 		 - 	none
//*****************************************************************************
void handle_d2(bool line_stopped)
{
	
	if(move_count_D2 == 0) {
		current_direction_d2 = get_new_direction(current_direction_d2);
		move_count_D2 = get_new_move_count(); 
	}
	
	// Get new direction and move count if next_move results in contact with defense boundary
	// While loop ensures that we check whether the new direction give to us after one pass through
	// does not result in the defense line moving pass its boundary
	while(defense_L2_boundary_reached(current_direction_d2, DEFENSE_2X_COORD, DEFENSE_2Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d2 = get_new_direction(current_direction_d2);
		move_count_D2 = get_new_move_count();
	}	

	move_count_D2--; 
	move_image(current_direction_d2, &DEFENSE_2X_COORD, &DEFENSE_2Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

//*****************************************************************************
// Handles movements of the defensive line 3, called from main() when
// ALERT_DEFENSE3 is set by timer
// 
// parameters  - 	line_stopped: indicates if the line has been stopped, passed 
// 															to move_image() function
// return 		 - 	none
//*****************************************************************************
void handle_d3(bool line_stopped) 
{
	if(move_count_D3 == 0) {
		current_direction_d3 = get_new_direction(current_direction_d3);
		move_count_D3 = get_new_move_count(); 
	}

	// Get new direction and move count if next_move results in contact with defense boundary
	// While loop ensures that we check whether the new direction give to us after one pass through
	// does not result in the defense line moving pass its boundary
	while(defense_L3_boundary_reached(current_direction_d3, DEFENSE_3X_COORD, DEFENSE_3Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels)) {
		current_direction_d3 = get_new_direction(current_direction_d3);
		move_count_D3 = get_new_move_count();
	}

	move_count_D3--; 
	move_image(current_direction_d3, &DEFENSE_3X_COORD, &DEFENSE_3Y_COORD, defense_playerHeightPixels, defense_playerWidthPixels, line_stopped);
}

//*****************************************************************************
// Checks if the coordinates of the offensive player overlap with any one of the
// defensive players on the screen. Updates the number of lives lost if overlap
// occurs. 
// 
// Only want to update number of lives lost if a) the offensive player overlaps
// with a defensive player that hasn't been cleared from the screen and b) if the
// overlap occurs with a defensive player that is seen on the screen based on the 
// the level the user is on in the game. 
// 
// 
// parameters  - 	level_reached: which level the user is currently on
// 						 - *cleared: address of cleared arrary, don't lose a life if
// 												 defensive player has been cleared from screen
// 						 - *lives_lost: address of number of lives_lost, update if overlap
// 														occurs with defensive player that hasn't been cleared
// 														from the screen
// return 		 - 	none
//*****************************************************************************
void update_lost_life(uint8_t level_reached, bool *cleared, uint8_t *lives_lost)
{
	
	bool life_lost = false; 
	// At most, check for overlap with every single defensive player on the screen (MAX 9 when level 3 reached).
	// Ensure that defensive player hasn't been cleared from screen, if so don't want to lose a life even if coordinate
	// overlap. 
  if(!cleared[0] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		life_lost = true; 
	}
	else if(!cleared[2] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD + 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD + 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		life_lost = true; 
	}
	
	else if(!cleared[1] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_1X_COORD - 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_1X_COORD - 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_1Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_1Y_COORD + (defense_playerHeightPixels/2))) 
	{
		life_lost = true;   
	}
	
	else if((level_reached >= 2) && !cleared[3] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{ 
		life_lost = true;   
	}
	else if((level_reached >= 2) && !cleared[5] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD + 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD + 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{ 		
		life_lost = true;   
	}
	
	else if((level_reached >= 2) && !cleared[4] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_2X_COORD - 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_2X_COORD - 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_2Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_2Y_COORD + (defense_playerHeightPixels/2))) 
	{
		life_lost = true;   
	}
	
	else if((level_reached >= 3) && !cleared[6] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{	
		life_lost = true; 
	}
	else if((level_reached >= 3) && !cleared[8] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD + 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD + 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{		
		life_lost = true;   
	}
	
	else if((level_reached >= 3) && !cleared[7] && !(OFFENSE_X_COORD - (offensive_PlayerWidthPixels/2) > DEFENSE_3X_COORD - 90 + (defense_playerWidthPixels/2) ||
			OFFENSE_X_COORD + (offensive_PlayerWidthPixels/2) < DEFENSE_3X_COORD - 90 - (defense_playerWidthPixels/2) ||
			OFFENSE_Y_COORD + (offensive_PlayerHeightPixels/2) < DEFENSE_3Y_COORD - (defense_playerHeightPixels/2) ||
			OFFENSE_Y_COORD - (offensive_PlayerHeightPixels/2) > DEFENSE_3Y_COORD + (defense_playerHeightPixels/2))) 
	{ 
		life_lost = true;   
	}
	
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
	}	
}

//*****************************************************************************
// Decreasing the number of lights illuminated on the io expander every button
// press by 2. If number of button presses is greater than 3, keep leds turned
// off. Number of leds indicates how many button presses the player has left. 
// Player gets 4 button presses total. 
// 
// parameters  - 	num_presses: number of button presses made by user
// return 		 - 	none
//*****************************************************************************
void update_io_leds(uint8_t num_presses)
{
	
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



//*****************************************************************************
// Handle whether to turn on or off the light after ALERT_BLINK has been set
// every 1 second by Timer1A
// 
// parameters  - 	none
// return 		 - 	none
//*****************************************************************************
void handle_blinking()
{
	static bool light_on; 
	// Handle blinking LED 0 every 1 second
		if(ALERT_BLINK)
		{
			if(!light_on) 
			{
				lp_io_set_pin(GREEN_BIT);
				light_on = true; 
			}
			else 
			{ 
				lp_io_clear_pin(GREEN_BIT);
				light_on = false; 
			}
			ALERT_BLINK = false;
		}
}
//*****************************************************************************
// Check based on the level reached and number of lives lost whether the game 
// is over. 
// 
// parameters  - 	lives_lost: number of lives lost
// 						 - 	level_reached: the level the user has reached
// return 		 - 	none
//*****************************************************************************
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



//*****************************************************************************
// Implement logic of the entire game
//*****************************************************************************
int main(void)
{
	
	uint32_t mode;  							// 4 Modes to the game, check main.h macros to know the four modes
	uint16_t lcd_x; 							// x coordinate of touch screen
	uint16_t lcd_y; 							// y coordinate of touch screen
	
	bool d_line_stop[3];  				// if set to true, that line doesn't move (i.e. if d_line_stop[0] == true, line 0 doesn't move)
	uint8_t num_line_left; 				// number of defensive lines left that can be stopped from moving on the screen
	uint8_t num_line_stopped; 		// number of defensive lines that have been stopped from moving on the screen
	uint8_t line_index; 					// index of the defensive lines
	
	bool dplayer_clear[9]; 				
	uint8_t num_dplayers_left;		// number of defensive players left that haven't been cleared
	uint8_t num_dplayers_cleared; // number of defensive players cleared off screen
	uint8_t dplayer_index; 				// index for the defensive players
	bool cleared[9]; 							// set once we have cleared a defensive player from the screen so that we don't continue drawing moving black rectangles throughout the screen
	
	uint8_t level_reached; 				// level the user is on - 3 different levels, increasing difficulty 
	static uint8_t lives_lost; 		// number of live_lost - allowed 2 lives for level 1, 3 lives for level 2, 4 lives for level 3
	
	// variables for push button logic, including debouncing
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
	
	while(1) {
		
		// Main menu mode: display high score, reset high score and display by touching the screen, 
		// start playing game by touching Play Game button
		while(mode == MAIN_MENU) {
			
			lcd_x = 0;
			lcd_y = 0;
			
			handle_blinking(); 						// blinking TIVA Launchpad LED
		
			// Determining if lcd screen has been touched 
			if (ft6x06_read_td_status())
			{
				// debounce the lcd touch - only detect touch once
				if(debounce_fsm_lcd(false)) 
				{
					lcd_x = ft6x06_read_x();
					lcd_y = ft6x06_read_y();
				}
			}
			else 
			{
				debounce_fsm_lcd(true); 
			}
			
			// If player touches the Reset High Score button, reset the high score to 0, write eeprom, and 
			// redisplay high score on the main menu. 
			if((lcd_x > RESET_LEFT) && (lcd_x < RESET_RIGHT) && (lcd_y > RESET_TOP) && (lcd_y < RESET_BOTTOM)) 
			{
				eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, 0); 
				lcd_clear_screen(LCD_COLOR_BLACK); 
				print_main_menu(); 
			}
			
			// Wait for player to touch the play game box to start playing the game, reset game variables necessary
			// for game to function. Begin Level 1.
			else if((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM)) 
			{
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
			
			handle_blinking();				// blinking TIVA Launchpad LED
			
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
				// Debounce the directional push buttons
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
						line_index = generate_random_number() % num_line_left; 
						// keep generating new line to stop until a line that hasn't been stopped is chosen
						while(d_line_stop[line_index] && (num_line_stopped < num_line_left)) 
						{
							line_index = generate_random_number() % num_line_left; 
						}
						d_line_stop[line_index] = true;
						num_line_stopped++; 
						num_presses++; 
						update_io_leds(num_presses); 																		// decrease the number of LEDs on by two
					}
					// When left is pressed, a random defensive player is cleared from the screen. 
					// If a defensive line has been stopped, a defensive player on that line could
					// be randomly chosen to get cleared, but because the line the player is on
					// is already stopped, the player won't get cleared.
					else if (left_pressed)
					{
						dplayer_index = generate_random_number() % num_dplayers_left;		
						// keep generating random player to cleared until a defensive player index is chosen 
						// that hasn't been cleared from the screen
						while(dplayer_clear[dplayer_index] && (num_dplayers_cleared < num_dplayers_left)) 
						{
							dplayer_index = generate_random_number() % num_dplayers_left; 
						}
						dplayer_clear[dplayer_index] = true; 
						num_dplayers_cleared++; 
						num_presses++; 
						update_io_leds(num_presses); 																		// decrease the number of LEDs on by two
					}
				}
			}
			
			
			// Logic to handle the movement of the offensive player based on PS2 Joystick reading
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
				// Check if life lost and if the game is over based on the movement and level reached
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
					mode = GAME_OVER; 
					break; 
				}
			}
			
			// Defense line 1 drawn on screen for every level, line 2 drawn on screen for level 2 and 3, and 
			// line 3 drawn on screen only for line 3. Three outer if statements for each line of defense. 
			// Logic for each line of defense is the exact same: 
			// 1) If defensive line hasn't been chosen to be stopped (when player presses down button), update 
			// 		coordinates of the defensive line and redraw. 
			// 2) If defensive player is randomly chose to be cleared (draw a black rectangle over the defensive 
			// 		player, and don't redraw the image after that - essentially cleared the defensive player
			// 3) After moving a defensive line, check if a life is lost and if the game is over
			// Purpose of array d_line_stop[] - index true if the corresponding line has been stopped
			// Purpose of array dplayer_clear[] - index is true if that defensive player has been chosen to be cleared
			// Purpose of array cleared[] 		- index true if that defensive player has been cleared, don't want to keep redrawing black rectangles
			// Again, logic is the same for each line of defense, only go into the defensive line's outer if statement
			// if the level the line shows up in is reached. 
			
			if(ALERT_DEFENSE1 && (level_reached >= 1))
			{
				ALERT_DEFENSE1 = false; 
				handle_d1(d_line_stop[0]); 
				// Only draw the line again (i.e. move the line) if the line has not been chosen to be stopped
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
						lcd_draw_image(DEFENSE_1X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[1]) 
					{
						cleared[1] = true; 
						lcd_draw_image(DEFENSE_1X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[2]) 
					{
						lcd_draw_image(DEFENSE_1X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[2]) 
					{
						cleared[2] = true; 
						lcd_draw_image(DEFENSE_1X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_1Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
			
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
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
						lcd_draw_image(DEFENSE_2X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[4]) 
					{
						cleared[4] = true; 
						lcd_draw_image(DEFENSE_2X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[5]) 
					{
						lcd_draw_image(DEFENSE_2X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[5])
					{
						cleared[5] = true; 
						lcd_draw_image(DEFENSE_2X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_2Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
				
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{
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
						lcd_draw_image(DEFENSE_3X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[7]) 
					{
						cleared[7] = true; 
						lcd_draw_image(DEFENSE_3X_COORD - 90, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					if(!dplayer_clear[8])
					{
						lcd_draw_image(DEFENSE_3X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
					}
					else if(!cleared[8])
					{
						cleared[8] = true; 
						lcd_draw_image(DEFENSE_3X_COORD + 90, defense_playerWidthPixels,
											 DEFENSE_3Y_COORD, defense_playerHeightPixels,
											 defense_playerBitMap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
				update_lost_life(level_reached, cleared, &lives_lost); 
				if(check_game_over(lives_lost, level_reached)) 
				{ 
					mode = GAME_OVER; 
					break; 
				}
			}	
			
			// Player has successfully reached other side of screen, move on to next level
			if(OFFENSE_Y_COORD >= 290) 
			{
				mode = LEVELED_UP; 
			}
		}	
		
		// 3. Level up menu displayed
		while(mode == LEVELED_UP) 
		{
			// 7 points gained for reaching other side of LCD screen, 1 point lost for 
			// each press of push button. 1 point lost for each life lost.  
			if(num_presses > 4)
				current_score = current_score + 3; 
			else
				current_score = current_score +  7 - num_presses; 
			
			if(current_score - lives_lost < 0)
				current_score = 0; 
			else
				current_score = current_score - lives_lost; 
			
			// Increase level, print level up screen screen
			lcd_clear_screen(LCD_COLOR_BLACK);
			print_level_up(); 
			level_reached++; 
			
			// Reset IO Expander LEDs to be all turned on
			write_leds(0xFF);
			
			// Reset number of lives lost
			lives_lost = 0; 
			printf("\n\rLeveled up!");
			
			// Reset offensive coordinates
			OFFENSE_X_COORD = 35; 
			OFFENSE_Y_COORD = 35;
			
			// Reset arrays and variables used for handling logic for 
			// stopping defensive lines or clearing defensive players when button pressed
			num_presses = 0; 
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
				num_line_left = 3; 
				num_dplayers_left = 9; 
			}
			// no more levels to go, game over
			else if(level_reached > 3) 	
			{
				lcd_clear_screen(LCD_COLOR_BLACK);
				mode = GAME_OVER;
				break; 
			}

			// Stay in level up menu until user pressed next level button pressed
			lcd_x = 0;
			lcd_y = 0;
			while(!((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM))) 
			{
				handle_blinking();						// blinking TIVA Launchpad LED
				if (ft6x06_read_td_status())
				{
					if(debounce_fsm_lcd(false)) 
					{
						lcd_x = ft6x06_read_x();
						lcd_y = ft6x06_read_y();
					}
				}
				else {
					debounce_fsm_lcd(true); 
				}
			}
			printf("\n\rPlaying the game"); 
			mode = GAME_IN_PROGRESS;  
			lcd_clear_screen(LCD_COLOR_BLACK);
			print_lives(lives_lost, level_reached); 
			lcd_draw_image(OFFENSE_X_COORD, offensive_PlayerWidthPixels,
										 OFFENSE_Y_COORD, offensive_PlayerHeightPixels,
										 offensive_PlayerBitmaps, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
		}
			
		// 4. Game over menu displayed 
		while(mode == GAME_OVER) 
		{
			print_game_over(); 
			// If new high score, write to eeprom
			if(current_score > high_score) 
			{
				eeprom_byte_write(I2C1_BASE, HIGH_SCORE_ADDRESS, current_score); 
			}
			
			// Reset IO Expander LEDs to be all turned on
			write_leds(0xFF);
			
			// Reset number of lives lost and score
			lives_lost = 0; 
			current_score = 0;
			
			// Reset offensive coordinates
			OFFENSE_X_COORD = 35; 
			OFFENSE_Y_COORD = 35;
			
			// Reset arrays and variables used for handling logic for 
			// stopping defensive lines or clearing defensive players when button pressed
			num_presses = 0;
			for(line_index = 0; line_index < 3; line_index++) 
			{
				d_line_stop[line_index] = false; 
			}
			
			for(dplayer_index = 0; dplayer_index < 9; dplayer_index++) 
			{
				dplayer_clear[dplayer_index] = false;
				cleared[dplayer_index] = false; 
			} 
			
			// Stay in game over menu until user pressed next play again button, then move to main menu
			lcd_x = 0;
			lcd_y = 0;
			while(!((lcd_x > PLAY_LEFT) && (lcd_x < PLAY_RIGHT) && (lcd_y > PLAY_TOP) && (lcd_y < PLAY_BOTTOM))) 
			{
				handle_blinking();							// blinking TIVA Launchpad LED
				if (ft6x06_read_td_status())
				{
					if(debounce_fsm_lcd(false)) 
					{
						lcd_x = ft6x06_read_x();
						lcd_y = ft6x06_read_y();
					}
				}
				else {
					debounce_fsm_lcd(true); 
				}
			} 
			mode = MAIN_MENU;
			printf("\n\r**************** WELCOME TO FOOTBALL STARS!! ****************"); 
			lcd_clear_screen(LCD_COLOR_BLACK); 
			print_main_menu();
		} 
	}
}

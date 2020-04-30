// Copyright (c) 2015-16, Joe Krachey
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
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "TM4C123.h"
#include "driver_defines.h"
#include "gpio_port.h"
#include "lcd.h"
#include "lcd_images.h"
#include "timers.h"
#include "ps2.h"
#include "launchpad_io.h"
#include "serial_debug.h"
#include "eeprom.h"
#include "ft6x06.h"
#include "ws2812b.h"
#include "io_expander.h"

#include "project_interrupts.h"
#include "project_hardware_init.h"
#include "project_images.h"
#include "fonts.h"
#include "debounce.h" 

// Application runs in four distinct modes below:
#define MAIN_MENU 					1
#define GAME_IN_PROGRESS		2
#define LEVELED_UP 					3
#define GAME_OVER 					4

//EEPROM High Score Address
#define HIGH_SCORE_ADDRESS 	256

// Play Touch Screen Boundaries - Main Menu
#define PLAY_TOP 						215	
#define PLAY_BOTTOM					245	
#define PLAY_LEFT					  28
#define PLAY_RIGHT					212

#define RESET_LEFT 					28
#define RESET_RIGHT 				212
#define RESET_TOP 					166
#define RESET_BOTTOM				194

extern uint8_t high_score; 
extern uint8_t current_score; 

extern volatile bool SPACE_BAR_HIT; 
extern bool PAUSED; 

extern volatile bool ALERT_BLINK; 
extern WS2812B_t LEDs[1]; 

extern volatile uint16_t OFFENSE_X_COORD;
extern volatile uint16_t OFFENSE_Y_COORD;
extern volatile bool ALERT_OFFENSE;

extern volatile uint16_t DEFENSE_1X_COORD;
extern volatile uint16_t DEFENSE_1Y_COORD; 
extern volatile bool ALERT_DEFENSE1; 
extern volatile int ALERT_D1_INT;

extern volatile uint16_t DEFENSE_2X_COORD;
extern volatile uint16_t DEFENSE_2Y_COORD;
extern volatile bool ALERT_DEFENSE2;

extern volatile uint16_t DEFENSE_3X_COORD;
extern volatile uint16_t DEFENSE_3Y_COORD;
extern volatile bool ALERT_DEFENSE3; 

extern volatile bool ALERT_PUSH; 

typedef enum{
  PS2_DIR_UP,
  PS2_DIR_DOWN,
  PS2_DIR_LEFT,
  PS2_DIR_RIGHT,
  PS2_DIR_CENTER,
  PS2_DIR_INIT,
} PS2_DIR_t;


extern volatile uint16_t PS2_X_DATA; 
extern volatile uint16_t PS2_Y_DATA; 
extern volatile PS2_DIR_t PS2_DIR; 
//*****************************************************************************
// Generates a random number
// https://en.wikipedia.org/wiki/Linear-feedback_shift_register  -- DO NOT MODIFY
//*****************************************************************************
uint16_t generate_random_number(void);

//*****************************************************************************
// Generates the the new direction and number of pixels  -- DO NOT MODIFY
//*****************************************************************************
PS2_DIR_t get_new_direction(PS2_DIR_t curr_direction);

//*****************************************************************************
// Generates the the new direction and number of pixels  -- DO NOT MODIFY
//*****************************************************************************
uint16_t get_new_move_count(void);

//extern void WS2812B_write(uint32_t port_base_addr, uint8_t *led_array_base_addr, uint16_t num_leds);

void init_LEDS(void);
void update_LED(void); 

//*****************************************************************************
// Function to easily print strings to the LCD
//
// parameters - x_start: where first character is placed in x-direction
// 						- y_start: where first character is placedin y-direction
// 						- print_message: string to print to LCD
// return     - none
//*****************************************************************************
void lcd_print_string(uint16_t x_start, uint16_t y_start, char *print_message,
											uint16_t fColor, uint16_t bColor, uint16_t font_size);  


//*****************************************************************************
// Display main menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_main_menu(void); 

//*****************************************************************************
// Display game over menu to the LCD screen by calling lcd_print_string() and 
// lcd_draw_image()
//
// parameters - none
// return     - none
//*****************************************************************************
void print_game_over(void); 


bool defense_L1_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
);
		

bool defense_L2_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
); 

bool defense_L3_boundary_reached
(
		volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
);

//*****************************************************************************
// Determines if any part of the image would be off the screen if the image
// is moved in the specified direction.
//*****************************************************************************
bool contact_edge(
    volatile PS2_DIR_t direction,
    volatile uint16_t x_coord, 
    volatile uint16_t y_coord, 
    uint8_t image_height, 
    uint8_t image_width
);

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
);
				
void update_io_leds(uint8_t num_presses);
#endif

#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

#include <stdbool.h>
#include <stdint.h>
#include "launchpad_io.h"
#include "io_expander.h"
#include "main.h"

// Four directional buttons correspond to these GPIOB pins on the IO expander
#define RIGHT_BUTTON_PIN			3
#define LEFT_BUTTON_PIN				2
#define DOWN_BUTTON_PIN				1
#define UP_BUTTON_PIN					0

// Debounce states
typedef enum 
{
  DEBOUNCE_ONE,
  DEBOUNCE_1ST_ZERO,
  DEBOUNCE_2ND_ZERO,
  DEBOUNCE_PRESSED
} DEBOUNCE_STATES;

//*****************************************************************************
// Debounce FSM for push buttons
//*****************************************************************************
extern bool debounce_fsm(DEBOUNCE_STATES *state, bool pin_logic_level);

//*****************************************************************************
// Debounce FSM for lcd touch screen
//*****************************************************************************
extern bool debounce_fsm_lcd(bool pin_logic_level); 


#endif
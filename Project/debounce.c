#include "debounce.h"

//*****************************************************************************
// Debounce FSM for push buttons
//*****************************************************************************
bool debounce_fsm(DEBOUNCE_STATES *state, bool pin_logic_level)
{
	
  switch (*state)
  {
    case DEBOUNCE_ONE:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_1ST_ZERO;
      break;
    }
    case DEBOUNCE_1ST_ZERO:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_2ND_ZERO;
      break;
    }
    case DEBOUNCE_2ND_ZERO:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_PRESSED;
      break;
    }
    case DEBOUNCE_PRESSED:
    {
      if (pin_logic_level){
				*state = DEBOUNCE_ONE;
				ALERT_PUSH = false; 
			}
      else *state = DEBOUNCE_PRESSED;
      break;
    }
    default: while (1) {}; 
  }
  
  if (*state == DEBOUNCE_2ND_ZERO) return true;
  else return false;
}

//*****************************************************************************
// Debounce FSM for lcd touch screen
//*****************************************************************************
bool debounce_fsm_lcd(bool pin_logic_level)
{
	static DEBOUNCE_STATES state = DEBOUNCE_ONE; 
	
  switch (state)
  {
    case DEBOUNCE_ONE:
    {
      if (pin_logic_level) state = DEBOUNCE_ONE;
      else state = DEBOUNCE_1ST_ZERO;
      break;
    }
    case DEBOUNCE_1ST_ZERO:
    {
      if (pin_logic_level) state = DEBOUNCE_ONE;
      else state = DEBOUNCE_2ND_ZERO;
      break;
    }
    case DEBOUNCE_2ND_ZERO:
    {
      if (pin_logic_level) state = DEBOUNCE_ONE;
      else state = DEBOUNCE_PRESSED;
      break;
    }
    case DEBOUNCE_PRESSED:
    {
      if (pin_logic_level){
				state = DEBOUNCE_ONE;
			}
      else state = DEBOUNCE_PRESSED;
      break;
    }
    default: while (1) {}; 
  }
  
  if (state == DEBOUNCE_2ND_ZERO) return true;
  else return false;
}


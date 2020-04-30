
#include "defense_randomize.h"
//#include "main.h"

static const uint16_t START_STATE = 0xACE7u;
static const uint16_t   MOVE_AMOUNT[] = {25, 50, 75, 100, 125, 150, 175, 200};
static const PS2_DIR_t  MOV_DIR[] = {PS2_DIR_UP, PS2_DIR_DOWN, PS2_DIR_LEFT, PS2_DIR_RIGHT};

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
// Generates the the new direction and number of pixels  -- DO NOT MODIFY
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
// Generates the the new direction and number of pixels  -- DO NOT MODIFY
//*****************************************************************************
uint16_t get_new_move_count(void)
{
    return MOVE_AMOUNT[generate_random_number()%8];
}

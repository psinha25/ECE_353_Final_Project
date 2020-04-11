#include "ws2812b.h"

/*******************************************************************************
* Function Name: rotate_ws2812bs
********************************************************************************
* Summary:
*
*   Rotates the WS2812B structs are rotates in the array by one position.  This
*   can be done by coping each WS2812B stuct to the next highest index in the 
*   array.  The last WS2812B struct should be moved to base[0]
*
*
* Parameters:
*   base         Base address of the WS2812B_t array
*   num_leds     Size number WS2812B_t structs in the array
*
* Return:
*  void
*
*******************************************************************************/
void ws2812b_rotate( WS2812B_t *base, uint8_t num_leds) {
	
	WS2812B_t temp = base[num_leds - 1]; 
	int i;
	for(i = num_leds - 2; i >= 0; i--) {
		base[i+1] = base[i]; 
	}
	base[0] = temp; 
}

/*******************************************************************************
* Function Name: pulse_ws2812b
********************************************************************************
* Summary:
*
*   Pulses the RED LED for a each WS2812B LED in a series of WS2812B LEDs.
*
*   The function will contain a static variable named direction that indicates 
*   if the value RED color codes are currently decrementing or incrementing.  
*
*   If the direction is incrementing AND the value of the red field is less than
*   0xFF, the function will increment the red filed by 1 for each structure in 
*   the array.  
*
*   If the direction is incrementing and the value of the red filed is equal to
*   0xFF, the function will change the direction to be counting down and decrement
*   the red filed by 1 for each structure in the array.  
*
*   If the direction is decrementing AND the value of the red field is greater than
*   0x00, the function will decrement the red filed by 1 for each structure in 
*   the array.  
*
*   If the direction is decrementing and the value of the red filed is equal to
*   0x00, the function will change the direction to be counting up and increment
*   the red filed by 1 for each structure in the array. 
*
* Parameters:
*   base         Base address of the WS2812B_t array
*   num_leds     Size number WS2812B_t structs in the array
*
* Return:
*  void
*
*******************************************************************************/
void ws2812b_pulse(WS2812B_t *base, uint8_t num_leds) {
	static int direction = 1; 	// 1 - increment, 0 - decrement
	
	int i; 
	for(i = 0; i < num_leds; i++) {
		if(base[i].red < 0xFF && direction) {
			base[i].red++; 
		} else if(base[i].red == 0xFF && direction) {
			direction = 0; 
			base[i].red--; 
		} else if(base[i].red > 0x00 && !direction)  {
			base[i].red--; 
		} else {
			direction = 1; 
			base[i].red++;
		}
	}
}
	
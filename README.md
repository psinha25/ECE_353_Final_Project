# ECE_353_Final_Project
### Prasoon Sinha and Charles Jamieson
## Gameplay
In football stars the goal of the game is to get your player to the opposite end of the field passed and increasing number of randomly moving defenders, a maximum of seven points re awarded for crossing the field. To start the game the player hits a start game button on the main menu. The joystick is used to move the player on the screen. To help the player reach the opposite end of the field some powers are included. Either the player can remove a random defender from the screen or prevent a random line of defenders from moving. The player can use a combination there of 4 time per level and each use decreases the number of red LEDs shining by 2 and the number of points by 1. The max score is recorded for future players.
## Peripheral and features used
EEPROM: Used to store the high score for future players
IO Expander: Used to control the 8 red LEDs marking the number of powers left
ADC: Used to read the joystick's position
UART: Used to pause the game and displaye game infomation
Touchscreen: Used to start the game and choose other on screen options

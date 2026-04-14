# Touch-Mii-Controller
GameCube Controller emulation via RP2040, using the MIKROE Touchpad 4 Click in place of a C-Stick (Meant for Wii games that heavily rely on using the pointer)

This implementation is SPECIFICALLY for the MIKROE Touchpad 4 Click.
L Trigger and R Trigger are both digital buttons (outputs as fully-pressed analog trigger) with this implementation.

Wiring (GPIO Pin Numbers) (All Buttons short GPIO directly to GND):
A: 2
B: 3
X: 4
Y: 5
START: 6
D-UP: 7
D-DOWN: 8
D-LEFT: 9
D-RIGHT: 10
Z: 11
L: 12
R: 13
LT: 14
LT: 15
Touchpad 4 Click SDA: 0
Touchpad 4 Click SCL: 1
Touchpad 4 Click INT: 17
Touchpad 4 Click RST: 18
Data (output to console): 22

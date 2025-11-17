# BikeGPS
This is the Arduino Code which operates the BikeGPS system.

## Components
This project uses a Sparkfun ESP32 Thing Plus as the microcontroller. Other components used are the NEO-6M GPS module, SD Card Breakout, and 16x2 LCD Display.
<img width="647" height="626" alt="Screenshot 2025-11-16 202620" src="https://github.com/user-attachments/assets/da5fc736-9ba5-4f97-bca4-bfdb9dd118d8" />

## System Function
<img width="790" height="282" alt="Screenshot 2025-11-16 202938" src="https://github.com/user-attachments/assets/514eb747-bba6-4de5-99c8-cc21e3ec1252" />
This system uses 5 states to function. First, when the system is turned on, it enters the “Init” state, where necessary variables are set to their default state, the LCD display is set up to show messages, and the SD card is prepared to create new files. The system then immediately enters the “Wait” state, where the system remains dormant until the user is ready to begin a ride. Once the user is ready, they press the green button on the system (“G”). This transitions the system to the “Create” state, where a new file is created and formatted so that data can be logged about the current ride. The system will then enter the “Track” state, where data points will be logged for every predecided time increment. I use a time increment of 1000ms because the maximum update rate of the NEO-6M is 1Hz. During the “Track” state, pressing the yellow button will allow the user to toggle through displaying the various stats such as speed, distance, and time. Once the user is ready to end their ride, they press the red button (“R”). This transitions the system into the “End” state, where the system ends the current file, and sets up the system for a new file to be created. The system then enters the “Wait” state again, and the cycle can repeat indefinitely.

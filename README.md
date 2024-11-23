# batterymon
Bluetooth Battery monitor for LiFePO4 BMS

By Alessandro Nesta 2024
Youtube channel: https://www.youtube.com/@alessandronestaadventures

This is a Battery monitor that connects directly to Battery BMS.
It has been tested on Eco-Worthy 12V 100Ah Battery, but it should work with most of the chinese made battery and BMS.

The project is inspired by: https://github.com/kolins-cz/Smart-BMS-Bluetooth-ESP32

LVGL has been used for the GUI.

Hardware: ESP32-2432S028 commonly known as CYD (Cheap Yellow Display)
The code can be easily adapted to any other touchscreen display.

The boot logo can be changed using the LVGL Image converter (https://lvgl.io/tools/imageconverter) and replacing ghe bootlogo.c file.
With this code, the boot image is the Wingamm logo, because I have a Wingamm camper. You can replace with any logo, just be sure to match your screen resolution.

This first version works with one battery only. It connects directly without any setup.
It goes to standby after 2 minutes and it wakes up if you touch the screen.
Standby current around 0,6 mA.

  Future plan:
    - Settings and preferences screen.
    - Possibility to monitor multiple batteries in parallel.

   Libraries version:
    lvgl by kisvegabor version 9.2.2
    TFT_eSPI by Bodmer version 2.5.43
    XPT2046_Touchscreen by Paul Stoffegen version 1.4

   Please note that you need to configure TFT_eSPI and LVGL library to match your Display configuration.
   In this repository you will find the files for my CYD display which is the version with also USB-C.

   The files are: User_Setup.h and lv_conf.h
    The path are: 
    "Your Sketchbook location"\libraries\TFT_eSPI\User_Setup.h
    "Your Sketchbook location"\libraries\lv_conf.h

   Please note that depending on your LCD, you may have different configuration for these 2 files.

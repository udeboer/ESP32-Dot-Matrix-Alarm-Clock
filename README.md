# README.md
This alarmclock project is created with the ESP32 IDF. Version 4.2. 

The dotmatrix displays are controlled by max7219 chips. The code is compatible with the multiple of 4 by 8x8 dotmatrix modules found on internet. (Switch rotation on in the code for these). A simple level converter is used for the 3.3 > 5v level of the max7219. Some people do not use these. But I had some problems.

The time is kept by SNTP and an additional ds3231 for backup. It is easy to modify the code to just use one of the options. 

A rotary encoder with switch function is used for controlling the alarm clock. 

Sounds are played through i2s. MAX98357 i2s dac/amp works. Sounds are wav files stored on the spiffs flash filesystem

A web interface allows for setting up all the clocksettings and chosing the alarm sounds.  Accessible /index.html (/ works). Network setup is done through /setup. idf.py menuconfig allows for setting the startup WiFi params. 

Most of the files are MIT licensed. Only the rotary encoder and some bugfixing for the NEWLIB library are licensed different. 

## Remarks
-Hardware pins used are defined in the header files. Try first with these pins. Other pins can give problems with uploading or running. 
-Most of the software can be used seperate.  
-Almost everything runs in FreeRTOS tasks. Lots of CPU time left
-Year >2038 support is not yet in IDF (NEWLIB) and compiler. Is possible. But enable 64bit time_t in idf.py menuconfig and needs toolchain with 64bit time_t. Tested with this enabled. 
-Network setup is all DHCP. Some fields are defined in the setup parameters. But not yet implemented.
-Default ip address of AP is 192.168.4.1.
-Web pages use JSON to communicate with the ESP32. See files in spiffs directory. And some tests with curl in tests.





/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef DISPLAY_TASK_H_
#define DISPLAY_TASK_H_

#define MENU_TIMOUT 8000
#define MENU_TIMEOUT_SHORT 1200
// alarm times is adjusted by 5 minutes interval steps.
#define ALARM_ADJUST 5

//During Alarm. Short press will sleep. Long press switches alarm off.
//This defines minimal duration for long press.
#define ALARM_LONG_PRESS 1500 
#define WIFIOPENAP_LONG_PRESS 5000 

//The name of the NVRAM var with the CLOCK settings
#define NVFLASH_CLOCKBLOB "clock"

void start_display_clock_task();

#endif

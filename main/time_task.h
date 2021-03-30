/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef TIMETASK_H_
#define TIMETASK_H_

#include <stdbool.h>
#include "time.h"

// max timeout in minutes when the ds3231 takes over as primary source
// ds3231 is always used on startup to set initial time
// Normally sntp sync happens every 60 minutes.
#define SNTP_MAX_TIME_NOSYNC 121

extern struct tm current_timeinfo;

// Starts the tasks that sends an event each minute 
void start_time_task();

// Start or stop sntp
void start_stop_sntp(bool action);

// Get the minutes since the last sntp sync
// Needs to be called once a minute to keep counting the minutes
int last_sntp_sync_min();

// Set the UTC time
void set_time(time_t new_time);

// Get the UTC time
time_t get_time();

#endif

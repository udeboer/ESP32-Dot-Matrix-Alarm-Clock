/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef EVENTHANDLER_H_
#define EVENTHANDLER_H_

#include "defaults_globals.h"
#include "esp_event.h"
#include "esp_log.h"

// start and stop the default event handler
void start_stop_default_event_handler(bool action);

#endif
